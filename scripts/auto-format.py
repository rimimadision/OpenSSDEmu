#!/usr/bin/env python3

# Copyright (c) 2022 Hongkang Xiong <xionghongkang2019@bupt.edu.cn>

from cmath import log
import sys
import os
import logging

if __name__=="__main__":
    # pre-check if we install clang-format and cpplint
    if 0 != os.system('which clang-format > /dev/null'):
        logging.error('Not found clang-format or not adding clang-format to system path')
        exit

    if 0 != os.system('which cpplint > /dev/null'):
        # TODO: need to add find cpplint from pydoc module and pip list
        # https://cloud.tencent.com/developer/article/1570670
        logging.error('Not found cpplint or not adding cpplint to system path')
        exit

    # first we format the code to Google style
    clang_cmd_str = 'clang-format -i -style=google '
    for i in range(len(sys.argv)):
        if i != 0:
            clang_cmd_str += sys.argv[i] + ' '
    os.system(clang_cmd_str)

    # then we use cpplint to analyse C code 
    cpplint_cmd_str = 'cpplint' + ' '
    for i in range(len(sys.argv)):
        if i != 0:
            cpplint_cmd_str += sys.argv[i] + ' '
    os.system(cpplint_cmd_str + '1>/dev/null 2> cpplint_log')

    # process cpplint log
    with open('cpplint_log', 'r') as r:
        lines = r.readlines()
    with open('cpplint_log', 'w') as w:
        for l in lines:
            if 'Using C-style' not in l:
                w.write(l)
    with open('cpplint_log', 'r') as r:
        lines = r.readlines()
        if len(lines) == 0:
            print('Format done, no issues found')
        else:
            print('Format done, total '+ str(len(lines)) + ' issues, check /scripts/cpplint_log for detail')