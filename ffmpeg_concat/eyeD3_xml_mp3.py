#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, sys, codecs, glob, re
import xml.dom.minidom
from xml.dom.minidom import parse
import eyeD3

sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)

def read_xml():
    xmlFileName = '8minsreading_2010.xml'
    dom1 = parse(xmlFileName)
    
    txtFileName = xmlFileName.split('.')[0] + '.txt'
    fout = codecs.open(txtFileName, mode='w', \
            encoding='utf-8')
    
    items = dom1.getElementsByTagName('item')
    for item in items:
        title = item.getElementsByTagName('title')[0].\
                firstChild.nodeValue
        link = item.getElementsByTagName('link')[0].\
                firstChild.nodeValue
        fileName = link.split('/')[-1].split('.')[0]
        description = item.getElementsByTagName(\
                'description')[0].firstChild.nodeValue
        description = description.split(' ')[0]
        description = description.strip()
        fout.write(fileName + '\t' + title + '\t' + \
                description + '\n')
        
    fout.close()


def sum_all():
    pat = re.compile(r'\d\d\d\d\d\d\t')
    table = {}
    txtFiles = ['8minsreading_2011.txt', 
            '8minsreading_2010.txt', 
            '8minsreading_2009.txt', 
            '8minsreading_2008.txt', 
            '8minsreading_2007.txt']
    for txtFile in txtFiles:    
        fin = codecs.open(txtFile, mode='r', 
                encoding='utf-8')
        for line in fin:
            line = line.strip()
            if not line:
                continue
            if pat.match(line):
                comps = line.split('\t')
                fileTitle = comps[0]
                title = comps[1]
                description = ''.join(comps[2:])
                table[fileTitle] = (fileTitle, title, 
                        description)
            else:
                (fileTitle, title, description) = \
                        table[fileTitle]
                description += line
                table[fileTitle] = (fileTitle, title, 
                        description)
        fin.close()
    
    fout = codecs.open('all.txt', mode='w', 
            encoding='utf-8')
    keys = table.keys()
    keys.sort()
    for k in keys:
        fileTitle, title, description = table[k]
        fout.write( fileTitle+'\t'+ title +'\t'+ 
                description +'\n' )
    fout.close()

table = {}
fin = codecs.open('all.txt', mode='r', encoding='utf-8')
for line in fin:
    comps = line.split('\t')
    fileTitle, title = comps[0], comps[1]
    description = ''.join(comps[2:])
    table[fileTitle] = (title, description)
fin.close()
    
mp3Files = glob.glob('*.mp3')
for mp3File in mp3Files:    
    fileTitle = mp3File.split('.')[0]
    title, description = table[fileTitle]
    tag = eyeD3.Tag()
    tag.link(mp3File)
    tag.setVersion(eyeD3.ID3_V2_3)
    tag.encoding='\x01'
    tag.setTitle(fileTitle+title )
    tag.setAlbum(u'開卷八分鐘')
    tag.setArtist(u'梁文道')
    tag.addLyrics(description)
    for frame in tag.frames: frame.encoding='\x01'
    tag.update()

