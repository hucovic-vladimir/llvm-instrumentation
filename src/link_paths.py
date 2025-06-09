#!/usr/bin/env python3
"""
A linker wrapper script which links the pre-compiled instrumentation code with the program.
This script should be passed to clang as a linker script.
"""
import os
import sys
import subprocess
import shutil
import platform
from pathlib import Path

def main():
   LINKER = "ld64.lld"
   
   # Get the directory where this Python script is located
   script_path = Path(__file__).parent.resolve()
   
   # Working directory is where the user invoked this script
   working_dir = Path.cwd()
   
   instcode_obj_file = script_path / "instrumentationCode.o"
   print(str(instcode_obj_file))
   
   # Save .pathinst/path_counters/ directory if it exists (relative to working directory)
   path_counters_working = working_dir / ".pathinst" / "path_counters"
   path_counters_script = script_path / ".pathinst" / "path_counters"
   
   if path_counters_working.exists():
       # Create the .pathinst directory in script path if it doesn't exist
       path_counters_script.parent.mkdir(parents=True, exist_ok=True)
       
       # Remove existing directory if it exists and copy the new one
       if path_counters_script.exists():
           shutil.rmtree(path_counters_script)
       shutil.copytree(path_counters_working, path_counters_script)
       print(f"Copied {path_counters_working} to {path_counters_script}")
   
   # Run make clean and make with Makefile_paths in the script directory
   try:
       subprocess.run(["make", "-f", "Makefile_paths", "clean"], cwd=script_path, check=True)
       subprocess.run(["make", "-f", "Makefile_paths"], cwd=script_path, check=True)
   except subprocess.CalledProcessError as e:
       print(f"Make command failed: {e}")
       sys.exit(1)
   
   # Check if the instrumentation object file exists
   if not instcode_obj_file.exists():
       print(f"Error: Instrumentation object file not found at {instcode_obj_file}")
       print("Compiling of instrumentation code probably failed.")
       sys.exit(1)
   
   # On macOS, get the SDK path for linking
   sdk_args = []
   if platform.system() == "Darwin":
       try:
           result = subprocess.run(
               ["xcrun", "--show-sdk-path"], 
               capture_output=True, 
               text=True, 
               check=True
           )
           sdk_lib = Path(result.stdout.strip()) / "usr" / "lib"
           sdk_args = [f"-L{sdk_lib}"]
       except subprocess.CalledProcessError:
           print("Warning: Could not get SDK path")
   
   # Run the actual linker
   linker_cmd = [LINKER] + sys.argv[1:] + sdk_args + [str(instcode_obj_file)]
   try:
       result = subprocess.run(linker_cmd)
       exit_code = result.returncode
   except FileNotFoundError:
       print(f"Error: Linker '{LINKER}' not found")
       sys.exit(1)
   
   # Cleanup - remove the copied directory
   if path_counters_script.exists():
       shutil.rmtree(path_counters_script)
       print(f"Cleaned up {path_counters_script}")
   
   sys.exit(exit_code)

if __name__ == "__main__":
   main()
