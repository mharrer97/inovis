import cv2
import os
import argparse

def parse_args():
    parser = argparse.ArgumentParser(description="resize images")
    parser.add_argument("-w", "--width" , type=int,  help="target_width", required=True)
    parser.add_argument("-he", "--height", type=int,  help="target_height", required=True)
    parser.add_argument('-i', "--in_path",  help="input path", required=True)
    parser.add_argument('-o', "--out_path",  help="output path", required=True)

    parser.add_argument('-te',"--take_every_nth" ,type=int, default=1, help="only take every x frame")
    parser.add_argument( '-ow', "--overwrite", action='store_true', help="overwrite images")

    args = parser.parse_args()
    return args

def main():
    args = parse_args()
    #dirpath = "./data/perspective_mask_544/"
    w = args.width
    h = args.height
    frompath = args.in_path
    topath = args.out_path
    take_every_nth = args.take_every_nth
    os.makedirs(topath, exist_ok=True)    
    id = 0
    
    for f_name in sorted(os.listdir(frompath)):
        
        if f_name[-4:] != ".jpg" and f_name[-4:] != ".png":
            continue
        if id % args.take_every_nth != 0: 
            id += 1
            continue
        print("process", f_name , end="\r")
        img=cv2.imread(os.path.join(frompath , f_name))
        
        h_off = (img.shape[0] - h) //2
        w_off = (img.shape[1] - w) //2
        #print(h_off,w_off)
        #print(img.shape)
        if h_off < 0:
            h_off=0
        if w_off < 0:
            w_off=0
        if h_off >0:
            img = img[h_off:-h_off]
        else:
            img = img[h_off:]
        #print(img.shape)
        if w_off >0:
            img = img[:,w_off:-w_off]
        else:
            img = img[:,w_off:]

        #print(img.shape)
        if not os.path.exists(os.path.join(topath , f_name)) or args.overwrite :
            cv2.imwrite(os.path.join(topath , f_name),img)
        id += 1



if __name__ == '__main__':
    main()

