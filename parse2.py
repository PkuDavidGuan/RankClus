rr = open('dblp_tmp_result.txt', 'r')
ww = open('dblp_result.txt', 'w+')
authorDist = {}
confDist = {}
while True:
    line = rr.readline()
    if not line:
        break
    content = line.split('$')
    if(len(content) < 2):
        continue
    
    for i in range(0, len(content)):
        content[i] = content[i].strip('\n').strip(' ')
    if not confDist.has_key(content[0]):
        confDist[content[0]] = 1
    else:
        confDist[content[0]] += 1

    for i in range(1,len(content)):
        if not authorDist.has_key(content[i]):
            authorDist[content[i]] = 1
        else:
            authorDist[content[i]] += 1


threshold = 5
count = 0
while True:
    count = 0
    if threshold > 15:
        break
    for Author in authorDist.keys():
        if authorDist[Author] > threshold and Author[0] >= 'A' and Author[0] <= 'Z':
            count += 1
    print('threshold:%d,count:%d'%(threshold,count))
    threshold += 1


rr.seek(0)
threshold = 14
while True:
    line = rr.readline()
    if not line:
        break
    content = line.split('$')
    if(len(content) < 2):
        continue
    for i in range(0, len(content)):
        content[i] = content[i].strip('\n').strip(' ')
    wFlag = False
    res = content[0]
    for i in range(1, len(content)):
        if authorDist[content[i]] > threshold and content[i][0] >= 'A' and content[i][0] <= 'Z':
            res = res + '$' + content[i]
            wFlag = True
    
    if wFlag:
        ww.write(res+'\n')
ww.close()
rr.close()
