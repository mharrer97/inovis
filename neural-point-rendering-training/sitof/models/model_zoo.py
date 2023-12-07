import torch
import torch.nn.functional as F
from torch.nn import init 
import torch.nn as nn
from torch import optim

from .models import *

class ModelZoo:
    def __init__(self, device):
        self.device = device
        self.models = {}
        self.models_init_params = {}
        self.optimizers = {}
        self.optimizer_init_params = {}
        self.schedulers = {}
        self.scheduler_init_params = {}

    def add_model(self, name: str, model_type : nn.Module, model_params: dict, init_type='normal'):
        print("Adding Model")
        self.models[name] = model_type(**model_params).to(self.device)
        if init_type != None:
            init_weights(self.models[name], init_type=init_type)

        self.models_init_params[name] = {'name':name, 'model_type': model_type, 'model_params': model_params, 'init_type':init_type}

        return self.models[name]

    def add_optimizer(self, name, models_to_optimize : list, learning_rate, optimizer_type=optim.Adam, optim_params :dict = {}):
        print("Adding Optimizer")
        m_params_map = [{'params' :  self.models[model_name].parameters()} for model_name in models_to_optimize]
        print(m_params_map)
        self.optimizers[name] = optimizer_type(m_params_map, lr=learning_rate, **optim_params)

        self.optimizer_init_params[name] = {'name':name, 'models_to_optimize': models_to_optimize, 'learning_rate': learning_rate, 'optimizer_type':optimizer_type,
        'optim_params' : optim_params}
        return self.optimizers[name]

    def add_scheduler(self, name, optimizer_name, scheduler_type = optim.lr_scheduler.ReduceLROnPlateau, scheduler_params :dict = {}):
        print("Adding Scheduler")
        self.schedulers[name] = scheduler_type(self.optimizers[optimizer_name], **scheduler_params)
        self.scheduler_init_params[name] = {'name' : name, 'optimizer_name' : optimizer_name, 'scheduler_type' : scheduler_type, 'scheduler_params' : scheduler_params}
        return self.schedulers[name]

    def save(self, filename:str):
        zoo_map = {}
        for k, v in vars(self).items():
            if k == 'models' or k == 'optimizers' or k == 'schedulers':
                state_dict_map = {name : torch_thing.state_dict() for name, torch_thing in v.items()}
                print('save state_dicts of',k)
                zoo_map[k] = state_dict_map
            else:
                zoo_map[k] = v 
        torch.save(zoo_map, filename)
        # TODO differentiate between models and rest (state_dict!!)

    def load_model(self, name, params, state_dict):
        self.models[name] = params['model_type'](**params['model_params']).to(self.device)
        print(self.models[name])
        self.models[name].load_state_dict(state_dict)

    def load_optimizer(self, name, params, state_dict):
        m_params_map = [{'params' :  self.models[model_name].parameters()} for model_name in params['models_to_optimize']]

        self.optimizers[name] = params['optimizer_type'](m_params_map, lr=params['learning_rate'], **params['optim_params'])
        self.optimizers[name].load_state_dict(state_dict)

    def load_scheduler(self, name, params, state_dict):
        self.schedulers[name] = params['scheduler_type'](self.optimizers[params['optimizer_name']], **params['scheduler_params'] )
        self.schedulers[name].load_state_dict(state_dict)

    def transfer_to_device(self, device):
        for m in self.models:
            self.models[m] = self.models[m].to(device)


    @classmethod
    def load(cls, filename, device='from_load'):
        print(filename)
        zoo_map = torch.load(filename)
        for k in zoo_map:
            print (k, " ")
        zoo = ModelZoo(zoo_map['device'])
        if not device == 'from_load':
            print("moving model zoo to device (careful, type is not checked) :", device)
            zoo.device = device

        # first, we init all the obejcts and parameters
        for k in vars(zoo):
            if hasattr(zoo, k):
                if not (k == 'models' or k == 'optimizers' or k == 'schedulers'):
                    setattr(zoo,k,zoo_map[k])
            else:
                print('WARNING:', k, 'is not a member of ModelZoo')

        # then, we load the state dicts
        for name, model_state_dict in zoo_map['models'].items():
            params = zoo.models_init_params[name]
            zoo.load_model(name, params, model_state_dict)  

        for name, optimizer_state_dict in zoo_map['optimizers'].items():
            params = zoo.optimizer_init_params[name]
            zoo.load_optimizer(name, params, optimizer_state_dict)

        for name, scheduler_state_dict in zoo_map['schedulers'].items():
            params = zoo.scheduler_init_params[name]
            zoo.load_scheduler(name, params, scheduler_state_dict)
        return zoo


def get_current_learning_rate(optimizer):
    # get current learing rate
    cur_lr = []
    for param_group in optimizer.param_groups:
        cur_lr.append(param_group['lr'])
    return cur_lr


        
