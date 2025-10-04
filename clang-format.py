from pathlib import Path
from wcmatch import glob
import subprocess

root = Path(".")  # change this to your root directory if needed
exts = {".cpp", ".h", ".hlsl"}
ignore_file = root / ".clang-format-ignore"

# Check for and load ignore file first 
# Yes, we manually use the patterns in the .clang-format-ignore file 
# because I can't get clang-format when invoked thru this python script
# to be aware of that file.
if not ignore_file.exists():
    print("Unable to find .clang-format-ignore")
    quit()

ignore_patterns = []
if ignore_file.exists():
    for line in ignore_file.read_text().splitlines():
        line = line.strip()
        if line and not line.startswith("#"):  # skip empty lines and comments
            ignore_patterns.append(line)

def is_ignored(path: Path) ->bool:
    rel_path = path.relative_to(root)
    rel_path_str = rel_path.as_posix() # ensure forward slashes 
    rel_path_str = rel_path_str + "/" # ensure a trailing slash is appended 
    return any(glob.globmatch(rel_path_str, pattern, flags=glob.GLOBSTAR) for pattern in ignore_patterns)

dirs = set()
for dir in root.rglob("*"):
    if dir.is_dir() and not is_ignored(dir):
        if any(file.is_file() and file.suffix in exts for file in dir.iterdir()):
            dirs.add(dir)

for ext in exts:
    #for dir in sorted(dirs):
    for dir in root.rglob("*"):
        if dir.is_dir() and not is_ignored(dir):
            if any(file.is_file() and file.suffix == ext for file in dir.iterdir()):
                try:
                    this_dir = "./" + str(dir.as_posix()) + "/*" + ext # Would look like: ./dir/*.cpp 
                    #print(this_dir)
                    subprocess.run(["clang-format", "-i", "--Werror", "--verbose", this_dir], check=True, text=True, shell=True, cwd='./')
                except subprocess.CalledProcessError as e:
                    errMsg = e.stderr or e.stdout
                    if errMsg is not None:
                        errMsg = errMsg.strip()
                        print("clang-format error encountered")
                        print(f"{errMsg}")
