import json
from deepdiff import DeepDiff
import re
from termcolor import colored

REFERENCE_PROFILE_NAME = "profile.json"
GENERATED_PROFILE = re.compile(r'profile_data_pid_.*json')

def print_test_results(number_failed, number_passed):
    if(number_failed > 0):
        print(colored(f"Some tests failed!", 'red'))
    else:
        print(colored(f"All tests passed!", 'green'))
    print(colored(f"Passed: {number_passed}", 'green'))
    print(colored(f"Failed: {number_failed}", 'red'))


def compare_json_files(filePathExpected, filePathActual, testName = "unnamed test"):
    print(filePathExpected)
    with open(filePathExpected, 'r') as file1:
        data1 = json.load(file1)

    with open(filePathActual, 'r') as file2:
        data2 = json.load(file2)

    diff = DeepDiff(data1, data2, verbose_level=2, ignore_order=True)
    if not diff:
        print(colored(f"{testName} Passed!", 'green'))
    else:
        print(colored(f"{testName} Failed!", 'red')) 
        print("Differences: ")
        print(diff)
    return diff
