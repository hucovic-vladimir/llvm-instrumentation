import json
from deepdiff import DeepDiff
import re

REFERENCE_PROFILE_NAME = "profile.json"
GENERATED_PROFILE = re.compile(r'profile_data_pid_.*json')

def compare_json_files(filePathExpected, filePathActual):
    print(filePathExpected)
    with open(filePathExpected, 'r') as file1:
        data1 = json.load(file1)

    with open(filePathActual, 'r') as file2:
        data2 = json.load(file2)

    diff = DeepDiff(data1, data2, verbose_level=2)
    return diff
