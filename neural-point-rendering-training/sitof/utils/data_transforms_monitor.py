import dill
import inspect
import os

## Monitor wrapping class for data transforms, used implicitly in the dataloader factory
## Transform Objects are wrapped and their code can be inspected and dumped.
## Also includes dill serialization (necessary for Lambda functions on Windows, and rumoured to be better on Linux)

class DataTransformsMonitor:    
    def __init__(self, transform):
        #if no object is given, store nothing
        if not type(transform) is type(None):
            self.code_lines = inspect.getsource(type(transform))
            #uncomment to print out data transforms
            #print(self.code_lines)
        self.transform = transform
                                        
    def get_code_of_transform(self):
        return self.code_lines

    def __call__(self,x):
        return self.transform(x)
