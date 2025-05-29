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

    # Save modules.tmp if it exists (relative to working directory)
    modules_tmp_working = working_dir / "modules.tmp"
    modules_tmp_script = script_path / "modules.tmp"

    if modules_tmp_working.exists():
        shutil.copy2(modules_tmp_working, modules_tmp_script)

    # Run make clean and make in the script directory
    try:
        subprocess.run(["make", "clean"], cwd=script_path, check=True)
        subprocess.run(["make"], cwd=script_path, check=True)
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

    # Cleanup
    if modules_tmp_script.exists():
        modules_tmp_script.unlink()

    sys.exit(exit_code)

if __name__ == "__main__":
    main()
