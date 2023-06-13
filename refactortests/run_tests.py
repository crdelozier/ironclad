import os
import subprocess
import sys

INCLUDE_DIR = "/home/delozier/ironclad/include/"
REFACTOR_TOOL = "/home/delozier/llvm-project/build/bin/ironclad-refactor"
EXTENSIONS = [".cpp",".c",".cxx",".cc"]

def run_test_case(input_file):
    try:
        file_and_ext = os.path.splitext(file)
        executable_name = file_and_ext[0]
        os.system("cp " + input_file + " " + input_file + ".copy")
        refactor_output = subprocess.check_output([REFACTOR_TOOL,input_file,"--"])
        print(refactor_output)
        os.system("cat " + input_file)

        compile_output = subprocess.check_output(["g++","-o",executable_name,input_file,"-I" + INCLUDE_DIR])
        print(compile_output)

        execute_output = subprocess.check_output(["./" + executable_name])
        print(execute_output)

        os.system("cat " + input_file)
        # Back up the refactored output
        os.system("cp " + input_file + " " + input_file + ".refactored")
        # Move the original back
        os.system("mv " + input_file + ".copy " + input_file)
    except subprocess.CalledProcessError as e:
        print("ERROR!")
        print(e.output)

if not len(sys.argv) > 1:
    printf("Usage: " + sys.argv[0] + " [directory]")
    sys.exit(1)
        
directory = sys.argv[1]

for file in os.listdir(directory):
    file_and_ext = os.path.splitext(file)
    if file_and_ext[1] in EXTENSIONS:
        home_directory = os.getcwd()
        os.chdir(directory)
        run_test_case(file)
        os.chdir(home_directory)
        
