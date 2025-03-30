import subprocess
import re
import pathlib
import testlib

def test_simple():
    dir = pathlib.Path(__file__).parent.absolute()
    subprocess.run(["make"], cwd=dir)
    subprocess.run(["./ccsds", "data/Baboon.pgm"], cwd=dir)
    profile_regex = re.compile(r'profile_data_pid_.*.json')
    actualProfilePath = [
        f for f in dir.iterdir() 
        if f.is_file()  and profile_regex.search(f.name)
    ][0]
    reference_profile = f"{dir}/{testlib.REFERENCE_PROFILE_NAME}"
    testlib.compare_json_files(reference_profile, actualProfilePath)
    subprocess.run(["make", "clean"], cwd=dir)

