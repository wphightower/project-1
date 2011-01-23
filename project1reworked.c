#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void tokenizer(char* str);
int containsChar(char* str, char space_delim);
void searchForArgs(char*** argv, char* buffer, int* size_of_arr);
int isEnvVar(char* word);
char* expandEnvVar(char* word);
char** parsePath();

int main()
{
	char hostname[25];
	gethostname(hostname, 25);
	while (1)
	{
		int bytes_read;
		int nbytes = 80;
		char *input;

		printf("%s@%s:%s:::D ", getlogin(), hostname, getcwd(NULL, 0));
		input = (char *) malloc (nbytes + 1);
		bytes_read = getline (&input, &nbytes, stdin);
		input[bytes_read-1] = '\0';
		
		if(strcmp(input, "exit") == 0)
		{
			free(input);
			return 1;
		}
		else
		{
			tokenizer(input);
		}

	}
}


void tokenizer(char* str)
{
	//=================
	//Check if input was empty
	//=================
	if(!strcmp(str, ""))
		return;

	char* space_delim =	" ";
	char* save;
	char* savestring = strdup(str);
	char* cmd = strtok_r(str, space_delim, &save);
	char** paths = parsePath();
	//===============================
	//built-ins
	//===============================
	//=================
	//CD
	//=================
	if (strcmp(cmd, "cd") == 0)
	{
		
		char* path = strtok_r(NULL, space_delim, &save);
		if(chdir(path)!= 0)
		{
			printf("%s: No such file or directory.\n", path);
		}
	}
	//=================
	//Viewproc **DOES NOT WORK***
	//=================
	else if (strcmp(cmd, "viewproc") == 0)
	{
		char* proc_delim = "\n ";
		/* Need to do input redirection for this to print contents.
		Should check whether the file is there or not properly*/
		char* file_name = strtok_r(NULL, proc_delim , &save);
		char* proc = "/proc/";
		//have to do some memory managment here for concatenation of strings
		int new_size = strlen(proc) + strlen(file_name);
		char* proc_file = malloc(new_size*sizeof(char));
		strcat(proc_file, proc);
		strcat(proc_file, file_name);
		
		int access_flag = access(proc_file, 1);
		if(access_flag == 0)
		{
			printf("%s exists\n", file_name);
		}
		else if(access_flag == -1)
		{
			printf("error: %s is not in /proc.\n", proc_file);
		}
		free(proc_file);
	}
	//=================
	//Echo
	//=================
	else if (strcmp(cmd, "echo") == 0)
	{
		char* word;
		for (word = strtok_r(NULL, space_delim, &save); word; word = strtok_r(NULL, space_delim, &save) )
		{
			if (isEnvVar(word) == 1)
			{
				char* val = expandEnvVar(word);
				printf("%s ", val);
			}
			else if(isEnvVar(word) == -1)			
			{
				printf("%s: Undefined variable.\n", word);
				return;
			}
			else
			{
				printf("%s ", word);
			}
		}
		printf("\n");
	}
	//=================
	//the rest
	//=================
	
	else
	{
		char* input_delim = "<";
		char* output_delim = ">";
		char* word;
		char** argv = NULL;
		char* command = strdup(savestring);
		char* input = NULL;
		char* output = NULL;
		int backgroundExec=0;
		
		//================
		//Check for background execution. If 1, then yes.
		//================
		if(containsChar(command, '&'))
		{
			if(command[strlen(command)-1] != '&')
			{
				printf("'&' may only occur at the end of a command line.\n");
				return;
			}
			else
			{
				backgroundExec=1;
			}
		}
		
		//================
		//Scan for input redirection. If it exists, split. 
		//First half goes into command. Second half goes into input.
		//================
		if (containsChar(command, '<'))
		{
			command = strtok_r(command, input_delim, &save);
			input = strtok_r(NULL, input_delim, &save);
		}
		
		//================
		//Scan for output redirection. Must scan both halves of above. 
		//Must first check to make sure input exists.
		//================
		if(containsChar(command, '>'))
		{
			command = strtok_r(command, output_delim, &save);
			output = strtok_r(NULL, output_delim, &save);
		}
		else if(input != NULL)
		{
			if(containsChar(input, '>'))
			{
				input = strtok_r(input, output_delim, &save);
				output = strtok_r(NULL, output_delim, &save);
			}
		}
		//================
		//Scan for arguments. Scan command, input and output(since we are unsure which may contain it). 
		//Save it into char* argv[].  char* argv[] initialized to 0. Add a null string at end.
		//Use realloc when adding a new argument
		//Append a null string at the end. This will act as a null terminator to a string-of-strings.
		//================
		int i = 0;
		char* flag_delim = " &";
		if(command != NULL)
		{
			command = strtok_r(command, flag_delim, &save);
			for (word = strtok_r(NULL, flag_delim, &save); word; word = strtok_r(NULL, flag_delim, &save) )
			{
				argv = realloc(argv, (i+1)* sizeof(char*));
				argv[i] = strdup(word);
				i++;
			}
		}
		if(input != NULL)
		{
			input = strtok_r(input, flag_delim, &save);
			for (word = strtok_r(NULL, flag_delim, &save); word; word = strtok_r(NULL, flag_delim, &save) )
			{
				argv = realloc(argv, (i+1)* sizeof(char*));
				argv[i] = strdup(word);
				i++;
			}
		}
		if(output != NULL)
		{
			output = strtok_r(output, flag_delim, &save);
			for (word = strtok_r(NULL, flag_delim, &save); word; word = strtok_r(NULL, flag_delim, &save) )
			{
				argv = realloc(argv, (i+1)* sizeof(char*));
				argv[i] = strdup(word);
				i++;
			}
		}
		argv = realloc(argv, i+1);
		argv[i] = 0;
		
		//================
		//Print parsing results. Perfect!
		//Remove this once we get job processing working
		//================
		
		if(command != NULL)
		{
			printf("Command: %s ", command);
		}
		if (argv != NULL)
		{
			printf("Flags: ");
			int j = 0;
			while(argv[j] != 0)
			{
				printf("%s ", argv[j]);
				j++;
			}
		}
		if (input != NULL)
		{
			printf("Input: %s ", input);
		}
		if (output!= NULL)
		{
			printf("Output: %s", output);
		}
		if(backgroundExec ==1)
		{
			printf("Background execution: yes");
		}
		printf("\n");	
		
	}
	return;
}

//==========
//Checks if a string contains a character
//==========			
int containsChar(char* str, char containee)
{
	int i=0;
	while(str[i]!='\0')
	{
		if(str[i] == containee)
		{
			return 1;
		}
		i++;
	}
	return 0;
}		

//==========
//Checks if the word is an environment variable.
//Returns 1 if it is a valid variable
//Returns -1 if it is in variable form, but is invalid
//Returns 0 if it is not in variable form
//Call this anywhere there may be an environment variable.
//Use this before attempting to expand an environment variable. 
//Must always check that it exists before attempting to expand, or will get a null
//==========

int isEnvVar(char* word)
{
	if (word[0] == '$')
	{
		char* env = &word[1];
		char* val = getenv(env);
		if(val==NULL)
		{
			return -1;
		}
		else
			return 1;
	}
	else
		return 0;
}
//==========
//Returns an expanded version of an environment variable
//==========
char* expandEnvVar(char* word)
{
	char* env = &word[1];
	char* val = getenv(env);
	return val;
}
				
//==========
//Searches for arguments in the command line
// ***DOES NOT WORK CURRENTLY. NOT RETURNING ARG LIST PROPERLY
//==========				
void searchForArgs(char*** argv, char* buffer, int* size_of_arr)
{
	char* word;
	char* flag_delim = " &";
	for (word = strtok_r(NULL, flag_delim, &buffer); word; word = strtok_r(NULL, flag_delim, &buffer) )
	{
		*argv = realloc(*argv, (*(size_of_arr)+1)*sizeof(char*));
		*argv[*(size_of_arr)] = strdup(word);
		printf("%s\n", *argv[*(size_of_arr)]);
		*(size_of_arr)++;
	}
	return;
}
	

//================
//Parsing $PATH
//Store into char* paths[].
//Follows same algorithm as flag processing
//Store a null string at the last slot to act as a null terminator.	
//================
char** parsePath()
{	
	char** paths = NULL;
	char* save = NULL;
	char* env_path = expandEnvVar("$PATH");
	char* path_delim = ":";
	int path_count = 0;
	char* path_word = NULL;
	for (path_word = strtok_r(env_path, path_delim, &save); path_word; path_word = strtok_r(NULL, path_delim, &save) )
	{
		paths = realloc(paths, (path_count+1)* sizeof(char*));
		paths[path_count] = strdup(path_word);
		path_count++;
	}
	paths[path_count]=0;
		/*Uncomment if need to debug path parsing.
		if (paths != NULL)
		{
			printf("Paths: ");
			int k = 0;
			while(paths[k] != 0)
			{
				printf("%s\n ", paths[k]);
				k++;
			}
		}*/		
	return paths;
}				
				
				
				
				
				