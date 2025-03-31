import subprocess
import re
import pathlib
import testlib

def run_on_image(image_file_name, expectedProfileName):
    dir = pathlib.Path(__file__).parent.absolute()
    subprocess.run(["./compress", image_file_name], cwd=dir)
    profile_regex = re.compile(r'profile_data_pid_.*.json')
    actualProfilePath = [
        f for f in dir.iterdir() 
        if f.is_file()  and profile_regex.search(f.name)
    ][0]

    reference_profile = f"{dir}/{expectedProfileName}"
    return testlib.compare_json_files(reference_profile, actualProfilePath, f"ccsds - {image_file_name}")

def test_ccsds():
    dir = pathlib.Path(__file__).parent.absolute()
    subprocess.run(["make"], cwd=dir)
    result = run_on_image(f"{dir}/data/Baboon.pgm", "profile_Baboon.json")
    subprocess.run(["make", "clean"], cwd=dir)
    return result

