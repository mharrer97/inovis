import torch

def get_ycbcr_tensor(rgb_tensor): #in range: [0..1]   out range: [0..1]
    #https://web.archive.org/web/20180421030430/http://www.equasys.de/colorconversion.html
    ycbcr_tensor = torch.clone(rgb_tensor)
    ycbcr_tensor[:,0,:,:] = ( 0.299 * rgb_tensor[:,0,:,:] + 0.587 * rgb_tensor[:,1,:,:] + 0.114 * rgb_tensor[:,2,:,:])*255
    ycbcr_tensor[:,1,:,:] = (-0.1689 * rgb_tensor[:,0,:,:] - 0.3316 * rgb_tensor[:,1,:,:] + 0.5005 * rgb_tensor[:,2,:,:])*255 + 128
    ycbcr_tensor[:,2,:,:] = ( 0.4998 * rgb_tensor[:,0,:,:] - 0.4185 * rgb_tensor[:,1,:,:] - 0.0812 * rgb_tensor[:,2,:,:])*255 + 128
    ycbcr_tensor = torch.clamp(ycbcr_tensor, min=0, max=255)
    ycbcr_tensor = ycbcr_tensor / 255
    return ycbcr_tensor

def get_rgb_tensor(ycbcr_tensor): #in range: [0..1]   out range: [0..1]
    #https://web.archive.org/web/20180421030430/http://www.equasys.de/colorconversion.html
    rgb_tensor = torch.clone(ycbcr_tensor)
    rgb_tensor[:,0,:,:] = (255 * ycbcr_tensor[:,0,:,:]) +                                           1.4   * (255*ycbcr_tensor[:,2,:,:]-128)
    rgb_tensor[:,1,:,:] = (255 * ycbcr_tensor[:,0,:,:]) - 0.343 * (255*ycbcr_tensor[:,1,:,:]-128) - 0.711 * (255*ycbcr_tensor[:,2,:,:]-128)
    rgb_tensor[:,2,:,:] = (255 * ycbcr_tensor[:,0,:,:]) + 1.765 * (255*ycbcr_tensor[:,1,:,:]-128) 
    rgb_tensor = torch.clamp(rgb_tensor, min=0, max=255)
    rgb_tensor = rgb_tensor / 255
    return rgb_tensor