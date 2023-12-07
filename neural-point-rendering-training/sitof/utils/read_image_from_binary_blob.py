from PIL import Image
import numpy as np
from struct import *
import time
from cv2 import threshold, THRESH_TRUNC, THRESH_TOZERO 

def read_next_bytes(fid, num_bytes, format_char_sequence, endian_character="<"):
    """Read and unpack the next bytes from a binary file.
    :param fid:
    :param num_bytes: Sum of combination of {2, 4, 8}, e.g. 2, 6, 16, 30, etc.
    :param format_char_sequence: List of {c, e, f, d, h, H, i, I, l, L, q, Q}.
    :param endian_character: Any of {@, =, <, >, !}
    :return: Tuple of read and unpacked values.
    """
    data = fid.read(num_bytes)
    return unpack(endian_character + format_char_sequence, data)

def read_blob_float32_deprecated(file_path):
    """ read binary blob from file_path with layout:
            width height channels 0 data
    """
    with open(file_path, "rb") as binary_file:
        # the first 3 numbers are width, height and channels
        # the fourth is just a delimiter to give a hint for correctness of the read file 
        width, height, channels, null_bytes = read_next_bytes(
                    binary_file, num_bytes=4*4, format_char_sequence="IIII")


        assert(height > 0)
        assert(width > 0) 
        assert(channels > 0)
        assert(null_bytes == 0)
        
        data = binary_file.read()
        vec3_list = [i[0] for i in iter_unpack('f', data)]

        if len(vec3_list) != width*height*channels:
            print(file_path, " ERROR: blob length mismatches width*height*channels!")
    

        arr = np.array(vec3_list, dtype=np.float32).reshape((height, width, channels))

        return arr
    
    print(file_path, " coudl not be opende!!!")


def read_blob(file_path):
    """ read binary blob from file_path with layout:

            [width] [height] [channels] [bytes_per_channel] 0 [data ...]
            
        if bytes_per_channel == 1 uint8 --> float32 (fixed point cast! div by 255)
        if bytes_per_channel == 2 float16 --> float32
        if bytes_per_channel == 4 float32 --> float32
    """
    with open(file_path, "rb") as binary_file:
        # the first 3 numbers are width, height and channels
        # the fourth is just a delimiter to give a hint for correctness of the read file 
        width, height, channels, bytes_per_channel, null_bytes = read_next_bytes(
                    binary_file, num_bytes=5*4, format_char_sequence="IIIII")


        assert(height > 0)
        assert(width > 0) 
        assert(channels > 0)
        assert(bytes_per_channel > 0)
        assert(null_bytes == 0)
        
        data = binary_file.read()
        if bytes_per_channel == 1:  # uint8 ("fixed precision" cast!)
            vec3_list = np.frombuffer(data, dtype=np.uint8)
            if len(vec3_list) != width*height*channels:
                print(file_path, " ERROR: blob length mismatches width*height*channels!")
            arr =  np.array(vec3_list, dtype=np.float32).reshape((height, width, channels))
            arr = arr/ 255.0
            return arr
        elif bytes_per_channel == 2: # half / float16
            vec3_list = np.frombuffer(data, dtype=np.float16)
            if len(vec3_list) != width*height*channels:
                print(file_path, " ERROR: blob length mismatches width*height*channels!")
            arr =  np.array(vec3_list, dtype=np.float32).reshape((height, width, channels))
            return arr
        elif (bytes_per_channel == 4): # float32
            vec3_list = np.frombuffer(data, dtype=np.float32)
            if len(vec3_list) != width*height*channels:
                print(file_path, " ERROR: blob length mismatches width*height*channels!")
            return vec3_list.reshape((height, width, channels))
        else:
            print(bytes_per_channel, " is not supported!!")
            exit()

    print(file_path, " coudl not be opende!!!")



def test_read_blob(imgpath,binpath, dst):
    arr = read_blob(binpath)
    print('bin max ', np.max(arr))
    # threshold input to prevent repeating the values on uint tranmsformation
    # -> image will be compared with cropped image of binary
    threshold(src=arr,dst=arr,thresh=1.0,type=THRESH_TRUNC, maxval = 1.00)
    threshold(src=arr,dst=arr,thresh=0.0,type=THRESH_TOZERO, maxval = 0.00)
    img_arr = (255*arr)
    
    img_arr = img_arr.astype(dtype=np.uint8)
    img = Image.fromarray(img_arr)
    img.save(dst+'bin.png')

    gt_img = Image.open(imgpath)
    gt_arr = np.array(gt_img) #.transpose(Image.FLIP_TOP_BOTTOM))
    Image.fromarray(gt_arr).save(dst+'gt.png')

    diff = np.abs(gt_arr.astype(dtype=np.float)- img_arr.astype(dtype=np.float)).astype(dtype=np.uint8)
    diff_img = Image.fromarray(diff)
    diff_img.save(dst+'diff.jpg')
    print('diff max ', np.max(diff))
    print(gt_arr.shape, np.min(gt_arr), np.max(gt_arr))
    print(img_arr[0,0])
    print(img_arr.shape, np.min(img_arr), np.max(img_arr))
    img = Image.fromarray(img_arr) # .transpose(Image.FLIP_TOP_BOTTOM)
    #img.save(dst+'bin.png')

# if __name__ == "__main__":
#     test_read_blob()
def show_blob(file_path):
    arr = read_blob(file_path)
    img_arr = (255*arr).astype(dtype=np.uint8)
    img = Image.fromarray(img_arr) #.transpose(Image.FLIP_TOP_BOTTOM)
    img.save('test_show_blob.png')
    img.show()
    


def main():
    col_path = "./testbinaryreading/col.png"
    colbin_path = "./testbinaryreading/col.png.bin"
    dst = "./testbinaryreading/"
    test_read_blob(col_path,colbin_path,dst)



if __name__ == '__main__':
    main()
