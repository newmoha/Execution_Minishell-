/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execution.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maskour <maskour@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/20 17:01:55 by maskour           #+#    #+#             */
/*   Updated: 2025/06/24 18:37:56 by maskour          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../minishell.h"

static void ignore_sigint(void)
{
	signal(SIGINT, SIG_IGN);
}

static void cmd_process(t_cmd *cmd, char **env)
{
	char *cmd_path;
	if (!cmd) { handle_cmd_errors(NULL, NULL); exit(1); }
	if (redirections(cmd)) exit(1);
	if (!cmd->cmd || !cmd->cmd[0]) { handle_cmd_errors(NULL, NULL); exit(1); }
	cmd_path = find_path(cmd->cmd[0], env);
	if (!cmd_path)
	{
		handle_cmd_errors(cmd->cmd[0], ": command not found\n");
		exit(127);
	}
	if (execve(cmd_path, cmd->cmd, env) == -1)
	{
		handle_cmd_errors(cmd_path, "execve failed\n");
		free(cmd_path);
		exit(126);
	}
	exit(0);
}

static void parent_process(pid_t id, int *status, t_shell *shell_ctx)
{
		waitpid(id, status, 0);
		restore_sigint();
		if (WIFEXITED(*status))
			shell_ctx->exit_status = WEXITSTATUS(*status);
		else if (WIFSIGNALED(*status))
		{
			shell_ctx->exit_status = 128 + WTERMSIG(*status);
			if (WTERMSIG(*status) == SIGINT)
				write (1,"\n",1);
		}
		else
			shell_ctx->exit_status = 1;
}

static void execute_single_command(t_cmd **cmd, char **envp, t_shell *shell_ctx)
{
	pid_t id;
	int status;
	
	ignore_sigint();
	id = fork();
	if (id == 0)
	{
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		cmd_process(*cmd, envp);
		shell_ctx->exit_status = 0;
	}
	else if (id > 0)
		parent_process(id, &status, shell_ctx);
	else
	{
		perror("fork failed");
		restore_sigint();
	}
}

static int count_cmd(t_cmd **cmd)
{
	t_cmd *current = *cmd;
	int i;
	
	i = 0;
	while (current)
	{
		i++;
		current = current->next;
	}
	return (i);
}
static t_cmd **arr_cmd(t_cmd **cmd, int cmd_count)
{
		t_cmd **cmd_arr = malloc(sizeof(t_cmd *) * (cmd_count + 1));
		if (!cmd_arr)
			return (NULL);
		t_cmd *current = *cmd;
		int i = -1;
		while ( ++i < cmd_count) 
		{
			cmd_arr[i] = current;
			current = current->next;
		}
		cmd_arr[cmd_count] = NULL;
		return (cmd_arr);
}
static int handle_multiple_cmd(char **env, t_cmd **cmd, int cmd_count, t_shell *shell_ctx)
{
		t_cmd **cmd_arr;

		cmd_arr = arr_cmd(cmd, cmd_count);
		if (!cmd_arr)
		{
			free(env);
			return (1);
		}
		execute_pipeline(cmd_arr, cmd_count, env, shell_ctx);
		free(cmd_arr);
		return (0);
}
int exicut(t_cmd **cmd, t_env **env_list, t_shell *shell_ctx)
{
	int cmd_count;
	if (!cmd || !*cmd || !env_list)
		return (1);
	char **env = convert(*env_list);
	if (!env)
		return (1);
	cmd_count = count_cmd(cmd);
	if (cmd_count == 1)
	{
		if (is_builtin((*cmd)->cmd[0]))
		{
			*env_list = execut_bultin(cmd, *env_list, shell_ctx);
			free_env(env);
			return (0);
		}
		execute_single_command(cmd, env, shell_ctx);
	}
	else
		handle_multiple_cmd(env, cmd, cmd_count, shell_ctx);
	free_env(env);
	return (0);
}
