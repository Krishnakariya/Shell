//GROUP NO: 34
//Fahad Nayyar: 2017049
//Krishna Kariya: 2017060
//Prachi Arora: 2017075

// for every malloc do free. For every malloc check error. 
// for every syscall chekc possible errors.
// think about return values.
#include <stdio.h> 
#include <unistd.h>  
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <stddef.h>
#include <signal.h>
#include <setjmp.h>

char * read_line();
char ** parse_line(char * line);
char *** pipe_parser(char ** input);
int execute_external_comand(char * cmd_name, char** cmd_args,char * input_file_path, char * output_file_path,char * error_file_path,int input_fd_read,int input_fd_write,int output_fd_read,int output_fd_write, int left_flag, int right_flag,int append_flag);
int pipe_executor(char *** cmd_names,int NoOfCommands,char * global_input_file_path, char * global_output_file_path,char * global_error_file_path,int append_flag);
char** redirection_parser(char ** input);
void SIGINT_handler(int sig);
void SIGUSR1_handler(int sig);
void shell_loop();


static sigjmp_buf restart_point;
pid_t childshell=-1;
pid_t *grandchild;
int NoOfCom =0;
int grand_child_no=0;

int main(int argc, char const *argv[])
{
	signal(SIGINT,SIGINT_handler); //syscall
	shell_loop();
	return 0;
}

void SIGUSR1_handler(int sig)
{
	int i=0;
	for (i = 0; i < grand_child_no; ++i)
	{
		kill(grandchild[i],SIGTERM); //syscall
		waitpid(grandchild[i],NULL,0); //syscall
	}
	_exit(1); //syscall
}
void SIGINT_handler(int sig)
{
	if (childshell>=0)
	{
		kill(childshell,SIGUSR1); //syscall
		waitpid(childshell,NULL,0); //syscall	
	}
	
	signal(SIGINT,SIGINT_handler); //syscall
	siglongjmp(restart_point,36); //syscall
}



void shell_loop()
{
	if (sigsetjmp(restart_point,1)==36) //syscall
	{
		printf("\n");	
	}	
	
	while (1)
	{
		printf("CSE231> ");
		char * input_line;
		char ** input_args;
		input_line = read_line();
		input_args = parse_line(input_line);
		if(strcmp(input_args[0],"cd")==0){
			if (chdir(input_args[1])==-1) //syscall
			{
				printf("%s is not a valid directory\n",input_args[1]);
			}	
			free(input_args);
			free(input_line);
			continue;
		}
		if (strcmp(input_args[0],"exit")==0)
		{
			_exit(0);
		}
		char *** arr = pipe_parser(input_args);
		char ** arr1 = redirection_parser(input_args);
		int status;
		pid_t pid = fork(); //syscall
		if (pid==0)
		{
			signal(SIGUSR1,SIGUSR1_handler); //syscall
			// printf("%d %s\n",strcmp(arr1[3],"a"),arr1[3]);
			if(strcmp(arr1[3],"a")==0)
			{
				status = pipe_executor(arr,NoOfCom,arr1[0],arr1[1],arr1[2],1);
			}else
			{
				status = pipe_executor(arr,NoOfCom,arr1[0],arr1[1],arr1[2],0);
			}
			_exit(1); //syscall
		}
		else if (pid>0)
		{
			childshell = pid;
			int yo = waitpid(pid,NULL,0); //syscall
			grand_child_no=0;
			NoOfCom=0;
			childshell=-1;
		}	
		else
		{
			printf("fork failed!\n");
		}

		if (status==1)
		{
			free(input_line);
			free(input_args);
			free(arr);
			free(arr1);
			free(grandchild);
		}
	}
}


char * read_line()
{
	char * ret_line = NULL;
	size_t buf_size = 0;
	size_t size_read;
	size_read = getline(&ret_line,&buf_size,stdin);
	return ret_line;
}

char ** parse_line(char * line)
{
	char ** ret_string_array = NULL;
	int array_size = 64;
	ret_string_array = (char **) malloc(array_size*(sizeof(char *)));
	if (ret_string_array == NULL)
	{
		printf("malloc failed! in parse_line\n");
	}	
	char * cur_word;
	int index=0;
	cur_word = strtok(line," \t\n");
	ret_string_array[index] = cur_word;
	index+=1;
	if (cur_word==NULL)
	{
		return ret_string_array;
	}
	
	while ((cur_word = strtok(NULL," \t\n"))!=NULL)
	{
		ret_string_array[index] = cur_word;
		index+=1;
		if (index>=array_size)
		{
			array_size+=64;
			ret_string_array = realloc(ret_string_array,(array_size)*sizeof(char *));
			if (ret_string_array == NULL)
			{
				printf("realloc failed! in parse_line\n");
			}	
		}
	}
	
	ret_string_array[index] = NULL;
	return ret_string_array;	
}


int execute_external_comand(char * cmd_name, char** cmd_args,char * input_file_path, char * output_file_path,char * error_file_path,int input_fd_read,int input_fd_write,int output_fd_read,int output_fd_write, int left_flag, int right_flag,int append_flag) 
{
	pid_t pid;
	int exec_ret;
	pid = fork(); //syscall
	if (pid == 0)
	{
		if(left_flag>=0)
		{
			close(0); //syscall
			dup(input_fd_read); //syscall
		}
		if(right_flag>=0)
		{
			close(1); //syscall
			dup(output_fd_write); //syscall 
		}
		close(input_fd_read); //syscall
		close(output_fd_read); //syscall 
		close(input_fd_write); //syscall
		close(output_fd_write); //syscall
		if (input_file_path!=NULL)
		{
			close(0); //syscall
			open(input_file_path,O_RDONLY);	 //syscall
		}
		if (output_file_path!=NULL)
		{
			close(1); //syscall
			// open(output_file_path, O_CREAT | O_WRONLY  | O_TRUNC, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			if(append_flag==1)
			{
				open(output_file_path, O_CREAT | O_WRONLY | O_APPEND, 0666); //syscall
			}
			else
			{
				open(output_file_path, O_CREAT | O_WRONLY | O_TRUNC, 0666); //syscall
			}
		}
		if(error_file_path!=NULL)
		{
			close(2); //syscall
			open(error_file_path, O_CREAT | O_WRONLY | O_TRUNC, 0666); //syscall
		}
		exec_ret = execvp(cmd_name,cmd_args); //syscall
	
		if (exec_ret<0)
		{
			
			fprintf(stderr,"exec failed!\n");
			return -1;
		}
	}
	else if (pid >0) 
	{ 
		grandchild[grand_child_no] = pid;
	}
	else 
	{ 
		printf("fork failed!\n");
		return -1;
	}

	return 1;

}

int pipe_executor(char *** cmd_names,int NoOfCommands,char * global_input_file_path, char * global_output_file_path,char * global_error_file_path,int append_flag)
{
	
	int fd1[2],fd2[2],i, exec_status;
	pipe(fd1); //syscall
	pipe(fd2); //syscall
	grandchild = (pid_t *)malloc((NoOfCommands+1)*(sizeof(pid_t)));
	if(grandchild==NULL){
		printf("malloc failed in pipe_executor\n");
	}
	if (NoOfCommands==1)
	{

		exec_status = execute_external_comand(cmd_names[0][0],cmd_names[0],global_input_file_path,global_output_file_path,global_error_file_path,fd2[0],fd2[1],fd1[0],fd1[1],-1,-1,append_flag);
		close(fd2[0]);  //syscall
		close(fd2[1]); //syscall
		close(fd1[0]); //syscall
		close(fd1[1]); //syscall
		wait(NULL); //syscall
		return exec_status;
		
	}	

	for (i = 0; i < NoOfCommands; i++)
	{
		
		grand_child_no=i;	
		if(i==0)
		{
			exec_status = execute_external_comand(cmd_names[0][0],cmd_names[0],global_input_file_path,NULL,global_error_file_path,fd2[0],fd2[1],fd1[0],fd1[1],-1,1,append_flag);
			close(fd2[0]); //syscall
			close(fd2[1]); //syscall
			pipe(fd2); //syscall
			
		}
		else if(i==NoOfCommands-1)
		{
			if (i%2==0)
			{
				exec_status = execute_external_comand(cmd_names[i][0],cmd_names[i],NULL,global_output_file_path,global_error_file_path,fd2[0],fd2[1],fd1[0],fd1[1],1,-1,append_flag);
				close(fd2[0]);  //syscall
				close(fd2[1]); //syscall
				close(fd1[0]); //syscall
				close(fd1[1]); //syscall
			}
			else
			{
				exec_status = execute_external_comand(cmd_names[i][0],cmd_names[i],NULL,global_output_file_path,global_error_file_path,fd1[0],fd1[1],fd2[0],fd2[1],1,-1,append_flag);
				close(fd1[0]);  //syscall
				close(fd1[1]); //syscall
				close(fd2[0]);  //syscall
				close(fd2[1]); //syscall
			}	
			
		}
		else
		{
			if (i%2==0)
			{
				exec_status = execute_external_comand(cmd_names[i][0],cmd_names[i],NULL,NULL,global_error_file_path,fd2[0],fd2[1],fd1[0],fd1[1],1,1,append_flag);
				close(fd2[0]); //syscall
				close(fd2[1]); //syscall
				pipe(fd2); //syscall
			}
			else
			{
				exec_status = execute_external_comand(cmd_names[i][0],cmd_names[i],NULL,NULL,global_error_file_path,fd1[0],fd1[1],fd2[0],fd2[1],1,1,append_flag);
				close(fd1[0]); //syscall
				close(fd1[1]); //syscall
				pipe(fd1); //syscall
			}	
		}
		
	

	if (exec_status==-1)
	{
		return -1;
	}	
	}

	for (i = 0 ; i < NoOfCommands ; i++ )
	{
		wait(NULL); //syscall
	}
	return exec_status;
}
char *** pipe_parser(char ** input)
{
	int anssize = 64;
	int commndlen = 64;
 	char ** p = input;
 	char *** ans = (char***)malloc(anssize*(sizeof(char**)));
 	if (ans == NULL)
 	{
 		printf("malloc failed! in pipe_parser\n");
 	}
 	int i=0,j=0;
 	ans[i] = (char **) malloc(commndlen*(sizeof(char*)));
 	if (ans[i] == NULL)
 	{
 		printf("malloc failed! in pipe_parser\n");
 	}
 	while(*p!=NULL)
 	{ 

 		if(strcmp(*p,"|")==0)
 		{
 			i+=1;
 			j=0;
 			if(i>=64)
 			{
 				anssize = anssize + 64;
 				ans  = (char ***) realloc(ans,anssize*(sizeof(char**)));
 				if (ans == NULL)
 				{
 					printf("realloc failed! in ans\n");
 				}
 			}
 			commndlen = 64;
 			ans[i] = (char **) malloc(commndlen*(sizeof(char*)));
 			if (ans[i] == NULL)
 			{
 				printf("malloc failed! in pipe_parser\n");
 			}
 		}
 		else if(strcmp(*p,"<")==0 || strcmp(*p,">")==0 || strcmp(*p,">>")==0) 
 		{
 			p+=2;
 			continue;
 		}

 		else if((**p=='1' && *(*p+1)=='>') || (**p=='2' && *(*p+1)=='>'))
 		{
 			p++;
 			continue;
 		}
 		else
 		{
 			if(j>=commndlen)
 			{
 				commndlen =commndlen + 64;
 				ans[i] = (char **) realloc(ans,commndlen*(sizeof(char*)));
 				if (ans[i] == NULL)
			 	{
			 		printf("realloc failed! in pipe_parser\n");
			 	}
 			}
 			ans[i][j] = *p;
 			j+=1;
 		}
 		p++;
 	}
 	NoOfCom = i+1;
 	return ans;
}

char** redirection_parser(char ** input)
{
	char ** p = input;
	char ** ans  =  (char **)malloc(4*(sizeof(char *)));
	if (ans == NULL)
 	{
 		printf("malloc failed! in redirection_parser\n");
 	}
	ans[0] = NULL;
	ans[1] = NULL;
	ans[2] = NULL;
	ans[3] = "o";
	char *c;
	while(*p!=NULL)
	{
		c = *p;
		if(strcmp(*p,">")==0)
		{
			ans[1] = *(p+1); 
			ans[3] = "o";
		}
		else if(strcmp(*p,"<")==0)
		{
			ans[0] = *(p+1);
		}
		else if(strcmp(*p,">>")==0)
		{
			ans[1] = *(p+1);
			ans[3] = "a";
		}
		if(*c=='1' && *(c+1) == '>')
		{
			ans[1] = c+2;
			ans[3] = "o";
		}
		if(*c=='2' && *(c+1) == '>')
		{
			if(strcmp(*p,"2>&1")==0)
			{
				ans[2] = ans[1];
			}
			else if(c+2 != NULL)
			{
				ans[2] = c+2;
			}
		}
		p++;
	}
	return ans;
}

