import os
import shutil
import re


def create_output_dir(file):
    try:
        shutil.rmtree(file)
    except:
        pass
    os.mkdir(file)



def get_latest_epoch_from_filename(str):
    print(str)
    f = re.findall('\d', str)
    print(f)
    return f[0]

def get_latest_checkpoint(directory):
    folders = [name for name in os.listdir(directory) if os.path.isdir(directory + name)]
    print(os.listdir(directory))
    print(folders)
    latest_checkpoint = max(folders, key=get_latest_epoch_from_filename)
    print(directory+latest_checkpoint)
    return directory+latest_checkpoint