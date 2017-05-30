rr = open('dblp.xml', 'r')
ww = open('dblp_tmp_result.txt', 'w+')

author = ""
while True:
    line = rr.readline()
    if not line:
        break
    if line.find("<inproceedings") == -1:
        continue
    paper = line
    while True:
        line = rr.readline()
        paper = paper + line
        if line.find("</inproceedings>") != -1:
            break
    
    try:
        year = paper.split("<year>")[1].split("</year>")[0]
    except:
        continue
    if int(year) < 1998 or int(year) > 2007:
        continue
    
    try:
        author = paper.split("<author>")[1].split("</author>")[0].strip('\n')
    except IndexError:
        continue
    paper = paper[paper.find("</author>")+len("</author>"):]
    while True:
        try:
            tmp = paper.split("<author>")[1].split("</author>")[0].strip('\n')
            paper = paper[paper.find("</author>")+len("</author>"):]
        except IndexError:
            break
        author = author + '$' + tmp
    try: 
        title = paper.split("<booktitle>")[1].split("</booktitle>")[0].strip('\n')
    except:
        continue
    res = title+'$'+author
    ww.write(res + '\n')
