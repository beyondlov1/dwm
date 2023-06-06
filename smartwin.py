import numpy as np
import word2vecmain


def create_adjacent_matrix(k):
    mat = np.zeros((k,k))
    mat[0,1] = mat[1,0] = 0.1
    mat[0,2] = mat[2,0] = 0.2
    mat[0,3] = mat[3,0] = 0.3
    mat[0,4] = mat[4,0] = 0.8
    mat[1,2] = mat[2,1] = 0.1
    mat[1,3] = mat[3,1] = 0.3
    mat[1,4] = mat[4,1] = 0.4
    mat[2,3] = mat[3,2] = 0.3
    mat[2,4] = mat[4,2] = 0.4
    mat[3,4] = mat[4,3] = 0.7
    return mat

def distance(v1, v2):
    return np.sqrt((v1[0] - v2[0])**2 + (v1[1] - v2[1]) **2)

def center_trans(old, center):
    return tuple(map(lambda x:x-center, old))

def center_itrans(old, center):
    return tuple(map(lambda x:x+center, old))

def has_decimal_part(fval):
    if fval - int(fval) > 0:
        return 1
    else:
        return 0

def create_adjacent_matrix_from(sentences_tuple, models):
    m = len(sentences_tuple[0])
    mats = []
    for k in range(len(models)):
        model = models[k]
        sentences = sentences_tuple[k]
        m = len(sentences)
        mat = np.zeros((m,m))
        for i in range(len(sentences)):
            for j in range(len(sentences)):
                if i == j:
                    mat[i,j] = 0
                    continue
                if sentences[i] in model.wv and sentences[j] in model.wv:
                    mat[i,j] = 1/(model.wv.similarity(sentences[i], sentences[j])+1)
                else:
                    mat[i,j] = 100
        mats.append(mat)
    
    print(mats)
    resultmat = np.ones((m,m))
    for mat in mats:
        resultmat = np.multiply(resultmat, mat)
    return resultmat


def resort(models, sentences_tuple):
    mat = create_adjacent_matrix_from(sentences_tuple,models)
    # print(mat)
    k,_ = mat.shape

    n = int((np.sqrt(k)-1)/2) + 1 + has_decimal_part((np.sqrt(k)-1)/2)
    center = n-1
    posmat = np.zeros((2*n-1,2*n-1)) - 1

    # print(n)
    remainindex = [i for i in range(k)]
    filled = [] #(index, i, j)

    # first
    firstindex = np.argmin(mat.sum(axis=0), axis=0) 
    # print(mat)
    # print(firstindex)
    # print(posmat)
    filled.append((firstindex, center, center))
    remainindex.remove(firstindex)
    posmat[center,center] = firstindex

    for a in range(len(remainindex)):
        minscore = float("inf")
        minitem = (-1,-1,-1) #(index, i, j)

        for level in range(0, n):
            for k in range(0, level+1):
                for g in range(0, level+1):
                    for sign in [(-1,1), (-1,-1),(1,1),(1,-1)]:
                        i = sign[0] * k
                        j = sign[1] * g
                        # print(i,j)
                        mi,mj = center_itrans((i,j),center)
                        # print(mi, mj)
                        if posmat[mi,mj] < 0:
                            for ri in remainindex:
                                score = 0
                                for filleditem in filled:
                                    findex = filleditem[0]
                                    fi = filleditem[1]
                                    fj = filleditem[2]
                                    score += distance((mi,mj), (fi, fj)) * mat[ri,findex]
                                if score < minscore:
                                    minscore = score
                                    minitem = (ri,mi,mj)
            if minscore != float("inf"):
                break
        filled.append(minitem)
        # print(minitem)
        remainindex.remove(minitem[0])
        posmat[minitem[1], minitem[2]] = minitem[0]
    

    # print(filled)
    # print(remainindex)
    # print(posmat)
    trace= [(0,0),(0,1),(-1,1),(-1,0),(-1,-1),(0,-1),(1,-1),(1,0),(1,1),(1,2),(0,2),(-1,2),(-2,2),(-2,1),(-2,0),(-2,-1),(-2,-2),(-1,-2),(0,-2),(1,-2),(2,-2),(2,-1),(2,0),(2,1),(2,2),(2,3),(1,3),(0,3),(-1,3),(-2,3),(-3,3),(-3,2),(-3,1),(-3,0),(-3,-1),(-3,-2),(-3,-3),(-2,-3),(-1,-3),(0,-3),(1,-3),(2,-3),(3,-3),(3,-2),(3,-1),(3,0),(3,1),(3,2),(3,3),(3,4),(2,4),(1,4),(0,4),(-1,4),(-2,4),(-3,4),(-4,4),(-4,3),(-4,2),(-4,1),(-4,0),(-4,-1),(-4,-2),(-4,-3),(-4,-4),(-3,-4),(-2,-4),(-1,-4),(0,-4),(1,-4),(2,-4),(3,-4),(4,-4),(4,-3),(4,-2),(4,-1),(4,0),(4,1),(4,2),(4,3),(4,4),(4,5),(3,5),(2,5),(1,5),(0,5),(-1,5),(-2,5),(-3,5),(-4,5),(-5,5),(-5,4),(-5,3),(-5,2),(-5,1),(-5,0),(-5,-1),(-5,-2),(-5,-3),(-5,-4),(-5,-5),(-4,-5),(-3,-5),(-2,-5),(-1,-5),(0,-5),(1,-5),(2,-5),(3,-5),(4,-5),(5,-5),(5,-4),(5,-3),(5,-2),(5,-1),(5,0),(5,1),(5,2),(5,3),(5,4),(5,5)]
    arrindexlist = []
    for atrace in trace:
        x = atrace[0] + center
        y = atrace[1] + center
        if x >= posmat.shape[0] or y >= posmat.shape[1]:
            break
        arrindexlist.append(int(posmat[x,y]))
    print(arrindexlist)
    return arrindexlist

def create_adjacent_matrix_from2(launchparents):

    remainindex = [i for i in range(len(launchparents))]
    chains = []
    for i in range(len(launchparents)):
        chain = [i]
        last = i
        for _ in range(len(launchparents)):
            if launchparents[last] in chain or launchparents[last] == -1:
                break
            else:
                chain.append(launchparents[last])
                last = launchparents[last]
        chains.append(chain)

    print(chains)

    m = len(launchparents)
    resultmat = np.zeros((m,m)) + 0.00001
    for i in range(len(chains)):
        chain = chains[i]
        for j in range(1,len(chain)):
            w = -np.log(j-1+1/np.e)
            resultmat[chain[0],chain[j]] = np.exp(1-j)
            resultmat[chain[j],chain[0]] = np.exp(1-j)
    print(resultmat)
    return resultmat;

    m = len(launchparents)
    resultmat = np.zeros((m,m)) + 0.00001
    # for i in range(m):
        # for j in range(m):
            # resultmat[i][j] = abs(i-j)
    for i in range(m):
        if launchparents[i] < 0:
            continue
        resultmat[i,launchparents[i]] = 1
        resultmat[launchparents[i],i] = 1
    print(resultmat)
    return resultmat

def resort2(tag,launchparents, ids:list):
    mat = create_adjacent_matrix_from2(launchparents)
    # print(mat)
    k,_ = mat.shape

    # n = int((np.sqrt(k)-1)/2) + 1 + has_decimal_part((np.sqrt(k)-1)/2)
    n = 6; # nlevel
    center = n-1
    posmat = np.zeros((2*n-1,2*n-1)) - 1

    # print(n)
    remainindex = [i for i in range(k)]
    filled = [] #(index, i, j)  i j 为绝对坐标

    # first
    # firstindex = np.argmin(mat.sum(axis=0), axis=0) 
    global lastfilledlistdict
    if tag not in lastfilledlistdict:
        lastfilledlist = []
        lastfilledlistdict[tag] = lastfilledlist
    else:
        lastfilledlist = lastfilledlistdict[tag]

    print(lastfilledlist)
    if len(lastfilledlist) > 0:
        for i in range(len(ids)):
            id = ids[i]
            for j in range(len(lastfilledlist)):
                lastfilled = lastfilledlist[j]
                if lastfilled[3] == id:
                    newlastfilled = (i, lastfilled[1], lastfilled[2], id)
                    filled.append(newlastfilled)
                    remainindex.remove(newlastfilled[0])
                    posmat[newlastfilled[1],newlastfilled[2]] = newlastfilled[0]
                    break
    else:
        firstindex = 0
        filled.append((firstindex, center, center, ids[0]))
        remainindex.remove(firstindex)
        posmat[center,center] = firstindex

    for index in range(len(remainindex)):
        # minscore = float("inf")
        # minitem = (-1,-1,-1) #(index, i, j)
        maxscore = float("-inf")
        maxitem = (-1,-1,-1,-1) #(index, i, j)

        filledlen = len(filled)
        filledn = int((np.sqrt(filledlen)-1)/2) + 1 + has_decimal_part((np.sqrt(filledlen)-1)/2)
        for level in range(0, filledn+1):
            for k in range(0, level+1):
                for g in range(0, level+1):
                    for sign in [(-1,1), (-1,-1),(1,1),(1,-1)]:
                        i = sign[0] * k
                        j = sign[1] * g
                        # print(i,j)
                        mi,mj = center_itrans((i,j),center)
                        # print(mi, mj)
                        if posmat[mi,mj] < 0:
                            for ri in remainindex:
                                score = 0
                                for filleditem in filled:
                                    findex = filleditem[0]
                                    fi = filleditem[1]
                                    fj = filleditem[2]
                                    score += (1/distance((mi,mj), (fi, fj))) * mat[ri,findex]
                                if score > maxscore:
                                    maxscore = score
                                    maxitem = (ri,mi,mj,ids[ri])
            # if maxscore != float("-inf"):
            #     break     
        filled.append(maxitem)
        # print(minitem)
        remainindex.remove(maxitem[0])
        posmat[maxitem[1], maxitem[2]] = maxitem[0]
    

    lastfilledlist = filled
    lastfilledlistdict[tag] = filled

    # print(filled)
    # print(remainindex)
    print(posmat)
    trace= [(0,0),(0,1),(-1,1),(-1,0),(-1,-1),(0,-1),(1,-1),(1,0),(1,1),(1,2),(0,2),(-1,2),(-2,2),(-2,1),(-2,0),(-2,-1),(-2,-2),(-1,-2),(0,-2),(1,-2),(2,-2),(2,-1),(2,0),(2,1),(2,2),(2,3),(1,3),(0,3),(-1,3),(-2,3),(-3,3),(-3,2),(-3,1),(-3,0),(-3,-1),(-3,-2),(-3,-3),(-2,-3),(-1,-3),(0,-3),(1,-3),(2,-3),(3,-3),(3,-2),(3,-1),(3,0),(3,1),(3,2),(3,3),(3,4),(2,4),(1,4),(0,4),(-1,4),(-2,4),(-3,4),(-4,4),(-4,3),(-4,2),(-4,1),(-4,0),(-4,-1),(-4,-2),(-4,-3),(-4,-4),(-3,-4),(-2,-4),(-1,-4),(0,-4),(1,-4),(2,-4),(3,-4),(4,-4),(4,-3),(4,-2),(4,-1),(4,0),(4,1),(4,2),(4,3),(4,4),(4,5),(3,5),(2,5),(1,5),(0,5),(-1,5),(-2,5),(-3,5),(-4,5),(-5,5),(-5,4),(-5,3),(-5,2),(-5,1),(-5,0),(-5,-1),(-5,-2),(-5,-3),(-5,-4),(-5,-5),(-4,-5),(-3,-5),(-2,-5),(-1,-5),(0,-5),(1,-5),(2,-5),(3,-5),(4,-5),(5,-5),(5,-4),(5,-3),(5,-2),(5,-1),(5,0),(5,1),(5,2),(5,3),(5,4),(5,5)]
    arrindexlist = []
    for atrace in trace:
        x = atrace[0] + center
        y = atrace[1] + center
        if x >= posmat.shape[0] or y >= posmat.shape[1]:
            break
        arrindexlist.append(int(posmat[x,y]))
    print(arrindexlist)
    return arrindexlist


def train():
    return word2vecmain.train()

def place(tag, pairs):
    n = 6; # nlevel
    center = n-1
    global lastfilledlistdict
    if tag not in lastfilledlistdict:
        lastfilledlist = []
        lastfilledlistdict[tag] = lastfilledlist
    else:
        lastfilledlist = lastfilledlistdict[tag]

    missedpairs = [pair for pair in  pairs]
    n = len(lastfilledlist)
    for i in range(n):
        filled = lastfilledlist[i]    
        for pair in pairs:
            if pair[0] == filled[0]:
                mi,mj = center_itrans((pair[1],pair[2]),center)
                print(f"{mi} {mj}")
                lastfilledlist[i] = (pair[0],mi,mj, pair[3])
                missedpairs.remove(pair)
                break

    for pair in missedpairs:
        mi,mj = center_itrans((pair[1],pair[2]),center)
        lastfilledlist.append((pair[0],mi,mj, pair[3]))
            


# resort(train(),(["windistance\n","dwm\n","C语言调用Python3实例_c调用python3_C5DX的博客-CSDN博客 — Mozilla Firefox\n","c-project\n"], ["St", "St", "firefox", "St"]))
lastfilledlistdict = {}
# resort2([-1,-1,-1,-1,-1,-1,-1,6])
