from itertools import count
from sklearn.model_selection import train_test_split
from sklearn.feature_extraction.text import CountVectorizer
from sklearn.naive_bayes import BernoulliNB
from sklearn.naive_bayes import GaussianNB
from sklearn.preprocessing import OneHotEncoder
import numpy as np
import jieba as jb
import gensim

def load_raw_dataset():
    with open("/home/beyond/action.log","r") as f:
        lines = f.readlines()
        if len(lines) <= 1:
            return []
        
        plines = []
        for line in lines:
            items = line.split("\t")
            time = int(items[0])
            action = items[1]
            winclass = items[2]
            winname = items[3].strip()
            plines.append((time, action, winclass, winname))

        plines = [line for line in plines if line[1] == "focus"]
        validplines = []
        for i in range(1,len(plines)):
            lastline = plines[i-1]
            curline = plines[i]

            # session, valid session => MIN_VALID_INTERVAL - MAX_VALID_INTERVAL, if > MAX_VALID_INTERVAL, next session
            MIN_VALID_INTERVAL = 2000
            MAX_VALID_INTERVAL = 5000
            interval =curline[0] - lastline[0] 
            if interval < MIN_VALID_INTERVAL:
                continue
            validplines.append((lastline[0], lastline[1], lastline[2], lastline[3], interval))

        return validplines

def skip(validplines, skipwindow):
    data = []
    tmpdatum = []
    for i in range(0, len(validplines) - skipwindow):
        for j in range(skipwindow):
            tmpdatum.append(validplines[i+j])
        data.append(tmpdatum)
        tmpdatum = []
    data = filter(lambda x:x[0]!=x[1],data)
    return list(data)

def create_raw_dataset(skippeddata):
    X = []
    y = []
    ccount = {}
    for datum in skippeddata:
        value = datum[1][2]+datum[1][3]
        # value = datum[1][2]
        if value in ccount:
            ccount[value] += 1
        else:
            ccount[value] = 1

    c = list(ccount.keys())
    sortedc = sorted(c, key=lambda x:ccount[x], reverse=True)
    yw2i = {}
    ywordlist = []
    for i in range(len(sortedc)):
        yw2i[sortedc[i]] = i
        ywordlist.append(sortedc[i])

    for i in range(len(skippeddata)):
        X.append([skippeddata[i][0][2], skippeddata[i][0][3]])
        y.append(yw2i[skippeddata[i][1][2]+skippeddata[i][1][3]])
        # X.append([skippeddata[i][0][2]])
        # y.append(yw2i[skippeddata[i][1][2]])
    y = np.array(y)
    return np.mat(X), y, yw2i, ywordlist

def create_name_dataset(X, y, yw2i, ywordlist):
    X = [[x[1]] for x in X]
    X = np.mat(X)
    X_enc, xw2i, wordlist = onehot(X, 0)
    return X_enc,y, xw2i, wordlist, yw2i, ywordlist

def create_class_dataset(X, y, yw2i, ywordlist):
    X = [[x[0]] for x in X]
    X = np.mat(X)
    X_enc, xw2i, wordlist = onehot(X, 0)
    return X_enc,y, xw2i, wordlist, yw2i, ywordlist

def cutstr(s):
    return list(jb.cut(s, cut_all=False))

def onehot(datasetmat,index):
    drugnamedict = {}
    drugnamecutlist = []
    m, n = datasetmat.shape
    for i in range(m):
        drugname = datasetmat[i, index]
        seglist = cutstr(drugname)
        drugnamecutlist.append(seglist)
        for seg in seglist:
            if seg in drugnamedict:
                drugnamedict[seg] += 1
            else:
                drugnamedict[seg] = 1

    drugnamelist = []
    for key in drugnamedict.keys():
        drugnamelist.append((key, drugnamedict[key]))

    drugnamelist = sorted(drugnamelist,key=lambda x:x[1],reverse=True)


    # print(drugnamelist[:10])

    word2index = {}
    wordlist = []
    for i in range(len(drugnamelist)):
        seg = drugnamelist[i][0]
        word2index[seg] = i
        wordlist.append(seg)

    encoded = np.zeros((m,len(word2index.keys())))
    for i in range(m):
        drugnamearr = drugnamecutlist[i]
        for cut in drugnamearr:
            index = word2index[cut]
            encoded[i][index] = 1

    return encoded, word2index, wordlist

def tostring(x, wordlist):
    result = ""
    i = 0
    for word in x:
        if word == 1:
            result += wordlist[i]
            result += " "
        i += 1
    return result

def train():
    skippeddata = skip(load_raw_dataset(), 2)
    class_sentences = list(map(lambda x: [x[0][2], x[1][2]] , skippeddata))
    name_sentences = list(map(lambda x: [x[0][3], x[1][3]] , skippeddata))
    # print(sentences)
    name_model = gensim.models.Word2Vec(name_sentences, min_count=3, window=2)
    class_model = gensim.models.Word2Vec(class_sentences, min_count=3, window=2)
    # print(model)
    # print(model.raw_vocab)
    # print(model.wv.key_to_index)
    # print(model.wv["winaction\n"])
    # print(model.predict_output_word(["winaction\n"], topn=3))
    # print(model.wv.most_similar("winaction\n", topn=3))
    return name_model, class_model 

# print(load_raw_dataset())
