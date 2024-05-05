#include <iostream>
#include <string>
#include <thread>
#include <functional>
#include <vector>
#include <semaphore.h>
#include <filesystem>
#include <cstdlib>
#include <cassert>
#include <sys/stat.h>
#include <fstream>
#include <regex>

using namespace std;
namespace fs = std::filesystem;

enum buffer_item
{
	buff_1,
	buff_2,
	buff_3,
	buff_4
};

struct ParsedArguments
{
	int buffer_size;
	int file_size;
	int uid;
	int gid;
	string pattern;
};

struct Buffer
{
	sem_t mutex;
	sem_t emptySlots;
	sem_t fullSlots;

	vector<string> data;
};

struct Stage
{
	string name;
	void (*thread)();
};

ParsedArguments parse_args(int argc, char *argv[]);

void filename_acquisition_thread();
void file_filter_thread();
void line_generator_thread();
void line_filter_thread();
void output_thread();

void safeAdd(string data, Buffer &buffer);
string safeRemove(Buffer &buffer);

const int BUFFERS = 4;
const int STAGES = 5;
const string COMPLETED = "WORKFLOW COMPLETE";

const vector<Stage> stages = {
	{"Stage 1: Filename Acquisition", filename_acquisition_thread},
	{"Stage 2: File Filter", file_filter_thread},
	{"Stage 3: Line Generator", line_generator_thread},
	{"Stage 4: Line Filter", line_filter_thread},
	{"Stage 5: Output", output_thread}};

vector<Buffer> buffers(BUFFERS);
vector<thread> threads(STAGES);

ParsedArguments config;

int main(int argc, char *argv[])
{
	config = parse_args(argc, argv);

	// initialize the buffer semaphores
	for (Buffer &buffer : buffers)
	{
		sem_init(&buffer.mutex, 0, 1);
		sem_init(&buffer.emptySlots, 0, config.buffer_size);
		sem_init(&buffer.fullSlots, 0, 0);
	}

	// initialize threads
	for (int i = 0; i < STAGES; i++)
	{
		threads[i] = thread(stages[i].thread);
	}

	// wait for all threads to terminate
	for (int i = 0; i < STAGES; i++)
	{
		threads[i].join();
	}

	for (Buffer &buffer : buffers)
	{
		sem_destroy(&buffer.mutex);
		sem_destroy(&buffer.emptySlots);
		sem_destroy(&buffer.fullSlots);
	}

	return 0;
}

// parses the command-line arguments: buffer_size, file_size, uid, gid, and pattern
// returns a ParsedArguments struct
ParsedArguments parse_args(int argc, char *argv[])
{
	ParsedArguments args;
	if (argc == 6)
	{
		args.buffer_size = stoi(argv[1]);
		args.file_size = stoi(argv[2]);
		args.uid = stoi(argv[3]);
		args.gid = stoi(argv[4]);
		args.pattern = argv[5];
	}
	else
	{
		cout << "Usage: pipegrep <buffer_size> <file_size> <uid> <gid> <pattern>" << endl;
		exit(1);
	}

	return args;
}

void safeAdd(string data, Buffer &buffer)
{
	/* Reserve an empty slot */
	sem_wait(&buffer.emptySlots);

	/* Acquire the lock for critical section */
	sem_wait(&buffer.mutex);

	assert(buffer.data.size() >= 0 && buffer.data.size() <= config.buffer_size);

	/* insert the item at the start of the buffer */
	buffer.data.insert(buffer.data.begin(), data);

	/* Wake up a consumer */
	sem_post(&buffer.fullSlots);

	/* Release the lock for critical section */
	sem_post(&buffer.mutex);

	return;
}

string safeRemove(Buffer &buffer)
{
	/* Reserve a full slot */
	sem_wait(&buffer.fullSlots);

	/* Acquire the lock for critical section */
	sem_wait(&buffer.mutex);

	assert(buffer.data.size() >= 0 && buffer.data.size() <= config.buffer_size);

	/* Delete an item at the end of the buffer */
	string data = buffer.data.back();
	buffer.data.pop_back();
	assert(data != "");

	/* Wake up a producer */
	sem_post(&buffer.emptySlots);

	/* Release the lock for critical section */
	sem_post(&buffer.mutex);

	return data;
}

// The thread in this stage recurses through the current directory and adds filenames to the first buffer
void filename_acquisition_thread()
{
	// Get the current working directory
	fs::path current_path = fs::current_path();

	// Recursively iterate over all the files in the current directory and its subdirectories
	for (const auto &entry : fs::recursive_directory_iterator(current_path))
	{
		// Check if the entry is a regular file
		if (entry.is_regular_file())
		{
			// Construct the relative path of the file
			fs::path relative_path = fs::relative(entry.path(), current_path);

			// Add to buffer
			string file_path = "./" + relative_path.string();
			safeAdd(file_path, buffers[buffer_item::buff_1]);
		}
	}

	// Add a "done" token to buff_1
	safeAdd(COMPLETED, buffers[buffer_item::buff_1]);
	return;
}

// In this stage, the thread will read filenames from buff1 and filter out files according to the values
// provided on the command-line for ⟨filesize⟩, ⟨uid⟩, and ⟨gid⟩ as described above. Those files not
// filtered out are added to buff2
void file_filter_thread()
{
	while (1)
	{
		string filename = safeRemove(buffers[buffer_item::buff_1]);

		if (filename == COMPLETED)
			break;

		struct stat file_stat;
		if (stat(filename.c_str(), &file_stat) == 0)
		{
			int file_size = file_stat.st_size;
			int uid = file_stat.st_uid;
			int gid = file_stat.st_gid;

			if (config.file_size != -1 && file_size <= config.file_size)
				continue;
			if (config.uid != -1 && uid != config.uid)
				continue;
			if (config.gid != -1 && gid != config.gid)
				continue;

			safeAdd(filename, buffers[buffer_item::buff_2]);
		}
		else
		{
			cerr << "Failed to get file information" << endl;
		}
	}

	// Add a "done" token to buff_2
	safeAdd(COMPLETED, buffers[buffer_item::buff_2]);

	return;
}

// The thread in this stage reads each filename from buff2 and adds the lines in this file to buff3.
void line_generator_thread()
{
	while (1)
	{
		string filename = safeRemove(buffers[buffer_item::buff_2]);

		if (filename == COMPLETED)
			break;

		// Open the file for reading
		ifstream file(filename);
		if (file.is_open())
		{
			string line;
			int line_number = 1; // Initialize line number
			while (getline(file, line))
			{
				// Add file name, line number, and line content to buff_3
				string line_with_info = filename + "(" + to_string(line_number++) + "):" + line;
				safeAdd(line_with_info, buffers[buffer_item::buff_3]);
			}
			file.close();
		}
		else
		{
			cerr << "Failed to open file: " << filename << endl;
		}
	}

	// Add a "done" token to buff_3
	safeAdd(COMPLETED, buffers[buffer_item::buff_3]);

	return;
}

// In this stage, the thread reads the lines from buff3 and determines if any given one contains ⟨string⟩
// in it. If it does, it adds the line to buff4.
void line_filter_thread()
{
	while (1)
	{
		string data = safeRemove(buffers[buffer_item::buff_3]);

		if (data == COMPLETED)
			break;

		// Split the data into file name, line number, and line content
		size_t pos = data.find('(');
		if (pos != string::npos && pos + 3 < data.size()) // Assuming ": " follows the line number
		{
			// Extract file name
			string filename = data.substr(0, pos);

			// Find the position of '(' to extract line number
			size_t line_pos = data.find('(');
			if (line_pos != string::npos)
			{
				// Extract line number
				string line_num_str = data.substr(line_pos + 1, data.find(')', line_pos) - line_pos - 1);
				int line_num = stoi(line_num_str);

				// Extract line content
				string line_content = data.substr(data.find(':') + 1); // Assuming ": " follows the line number

				// Check if the line content contains the pattern
				if (regex_search(line_content, regex(config.pattern)))
				{
					// If it does, add the line to buff_4
					safeAdd(filename + "(" + line_num_str + "): " + line_content, buffers[buffer_item::buff_4]);
				}
			}
		}
	}

	// Add a "done" token to buff_4
	safeAdd(COMPLETED, buffers[buffer_item::buff_4]);

	return;
}

// In the final stage, the thread simply removes lines from buff4 and prints them to stdout. Also, you
// need to figure out when to exit the program. How do you know when you got the last line? Hint:
// you can use a “done” token (would not work if you had multiple threads in a stage).
void output_thread()
{
	int cout_count = 0;

	while (1)
	{
		string data = safeRemove(buffers[buffer_item::buff_4]);

		if (data == COMPLETED)
			break;

		// Print file name, line number, and line content
		cout << data << endl;
		cout_count++;
	}

	// Print the total number of cout calls
	cout << "***** You found " << cout_count << " matches *****" << endl;

	return;
}
