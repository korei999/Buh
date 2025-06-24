#include "exec.hh"

#include "adt/logs.hh"

#include "sys/wait.h"

using namespace adt;

namespace exec
{

config::String64
runReadOutput(const config::StatusEntry& entry)
{
    char aBuff[512] {};

    ADT_ASSERT(entry.eType == config::StatusEntry::TYPE::EXEC, "type: {}", int(entry.eType));

    int aPipeFds[2] {};
    if (pipe(aPipeFds) == -1)
    {
        print::err("pipe() failed ({})\n", strerror(errno));
        return {};
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        print::err("fork() failed ({})\n", strerror(errno));
        return {};
    }

    if (pid == 0) /* child branch */
    {
        close(aPipeFds[0]);
        dup2(aPipeFds[1], STDOUT_FILENO);
        close(aPipeFds[1]);

        if (!entry.vArgs.empty())
            execvp(entry.vArgs[0], entry.vArgs.data());

        /* Just quit if execvp() somehow failed. */
        exit(1);
    }
    else /* parent branch */
    {
        close(aPipeFds[1]);
        read(aPipeFds[0], aBuff, sizeof(aBuff) - 1);

        close(aPipeFds[0]);
        int waitStatus = 0;
        waitpid(pid, &waitStatus, 0);

        if (!WIFEXITED(waitStatus) || WEXITSTATUS(waitStatus) != 0)
            LOG("child exited with status: {}\n", WEXITSTATUS(waitStatus));

        return entry.func.pfnFormatString(aBuff);
    }

    return {};
}

} /* namespace exec */
