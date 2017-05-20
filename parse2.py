rr = open('dblp_parse_result.txt', 'r')
ww = open('dblp_parse_2.txt', 'w+')
authorDist = {}

while True:
    line = rr.readline()
    if not line:
        break
    content = line.split(',')
    if(len(content) < 2):
        continue
    
    for i in range(1,len(content)):
        if not authorDist.has_key(content[i]):
            authorDist[content[i]] = 1
        else:
            authorDist[content[i]] += 1

print("authorNum:%d" % (len(authorDist)))

'''
threshold = 5
count = 0
while True:
    count = 0
    if threshold > 15:
        break
    for i in authorDist.keys():
        if authorDist[i] > threshold:
            count += 1
    print('threshold:%d,count:%d'%(threshold,count))
    threshold += 1
'''

rr.seek(0)
threshold = 12
while True:
    line = rr.readline()
    if not line:
        break
    content = line.split(',')
    if(len(content) < 2):
        continue
    
    wFlag = False
    res = content[0]
    for i in range(1, len(content)):
        if authorDist[content[i]] >= threshold and content[i][0] >= 'A' and content[i][0] <= 'Z':
            res = res + ',' + content[i]
            wFlag = True
    
    if wFlag:
        ww.write(res+'\n')