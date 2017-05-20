# -*- coding:utf-8 -*-
import xml.parsers.expat
import sys
import io

res = ''
xmlType = ''
yearFlag = False
conFlag = False
ww=open('dblp_parse_result.txt','w+')
def start_element(name, attrs):
    global res,xmlType,yearFlag,conFlag
    xmlType = name
    if name == "inproceedings":
        res = ''
        conFlag = True

def end_element(name):
    global res,xmlType,yearFlag,ww,conFlag
    if name == "inproceedings":
        if yearFlag:
            #print res
            ww.write(res+'\n')
        yearFlag = conFlag = False

def char_data(data):
    if(data == "\n"):
        return
    global res,xmlType,yearFlag,conFlag
    if conFlag and xmlType == 'year' and int(data) >= 1998 and int(data) <= 2007:
        yearFlag = True
    elif conFlag and xmlType == 'author':
        if(res == ''):
            res = data
        else:
            res = data + ',' + res
    elif conFlag and xmlType == 'booktitle':
        res = data + ',' + res

p = xml.parsers.expat.ParserCreate()

p.StartElementHandler = start_element
p.EndElementHandler = end_element
p.CharacterDataHandler = char_data
rr = open('dblp.xml', 'r')
p.ParseFile(rr)
ww.close()