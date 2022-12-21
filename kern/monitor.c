// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	u32 narg;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", 0, mon_help },
	{ "kerninfo", "Display information about the kernel", 0, mon_kerninfo },
	{ "backtrace", "backtrace function call", 0, mon_backtrace},
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	cprintf("stack backtrace\n");
	u32 ebp = read_ebp();
	while(1)
	{
		if (ebp != 0)
		{
			u32 *p = (u32*)ebp;
			u32 eip = *(p+1);
			u32 argc = 5;
			struct Eipdebuginfo info = {0};
			debuginfo_eip(eip, &info);

			if (info.eip_fn_narg != 0)
			{
				argc = info.eip_fn_narg;
				cprintf("argc %d", argc);
			}
			cprintf("ebp 0x%x eip 0x%x args", ebp, eip);
			
			for (int i = 0; i < argc; i++)
			{
				u32 arg = *(p+2+i);
				cprintf(" %8x", arg);
			} 
			cprintf("\n");
			cprintf("%s:", info.eip_file);
			cprintf("%d", info.eip_line);
			cprintf(": %.*s", info.eip_fn_namelen, info.eip_fn_name);
			cprintf(" %d\n", (int)(eip - info.eip_fn_addr));
			ebp = *(u32*)ebp;
		}
		else
		{
			break;
		}

	}	
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
		{
			if (argc != commands[i].narg)
			{
				cprintf("parameter count mismatch, expect %d parameter\n", commands[i].narg);
				return 0;
			}
			else
			{
				return commands[i].func(argc, argv, tf);
			}
		}
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}


/************************************************************************/
int string2value(char *str)
{
	char *p = str;
	while (*p == ' ' || *p == '\t')
		p++;
	int hex = 0;
	int temp_val = 0;
	int sum = 0;
	int base = 10;
	if (*p == '0')
	{
		if (*(p+1) == 0)
		{
			return 0;
		}
		else if (*(p+1) == 'x')
		{
			hex = 1;
			p += 2;
		}
	}
	if (hex)
	{
		base = 16;
		while(*p)
		{
			temp_val = *p - '0';
			if (temp_val >= 0 && temp_val<= 9)
			{
				sum = sum * base + temp_val;
			}
			else if (temp_val >= ('a'-'0') && temp_val <= ('f'-'0'))
			{
				sum = sum * base + temp_val - 39;
			}
			else
			{
				cprintf("%c is not a digital character");
				return -1;
			}
			p++;
		}
	}
	else
	{
		while(*p)
		{
			//cprintf("ptr %d\n", *p);
			temp_val = *p - '0';
			//cprintf("temp %d\n", temp_val);
			if (temp_val >= 0 && temp_val <= 9)
				sum = sum * base + temp_val;
			else
				return -1;
			p++;
		}
	} 
	return sum;
}