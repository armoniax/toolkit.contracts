#!/usr/bin/python
# -*- coding: UTF-8 -*-


import re;
import sys;

#定义函数
def save_string_to_file(string, filename):
    with open(filename, 'a') as file:
        file.write(string)
def fenge(str):
    str2=''
    for i in range(0, len(str), 2):
	    str2= str2+ '0x' +str[i:i+2] +','
    print(str2[:-1])
    
    
# print('publicKey')
# fenge('393098a8a69a77385428559dbd72608c11dd28369e8a5fcf19608c0eadc82535')
# print('message')
# fenge('3542be216769ccb882a4561771358dc0339ba734c5f8d5cc271da08aea2e5482')
print('signature')
fenge('f445222c81ae41f9f6e1d94ad47f6c58978e362cc1b6488d455ffea4a0c61c11748ea48b3d0e363b375863a2aefc49b37d9711919bfd708121c4dcc91daf850d')