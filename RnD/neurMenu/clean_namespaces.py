import sys
import subprocess
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

def custom_spawn(sh, escape, cmd, args, env_vars):
    cmd_args = [str(arg) for arg in args]
    
    proc = subprocess.Popen(
        cmd_args, 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE, 
        text=True,
        env=env_vars
    )
    stdout, stderr = proc.communicate()
    
    if stdout:
        sys.stdout.write(stdout.replace("oneMenu::", ""))
    if stderr:
        sys.stderr.write(stderr.replace("oneMenu::", ""))
        
    return proc.returncode

env.Replace(SPAWN=custom_spawn)
