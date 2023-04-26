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
    return np.log((v1[0] - v2[0])**2 + (v1[1] - v2[1]) **2)

def center_trans(old, center):
    return tuple(map(lambda x:x-center, old))

def center_itrans(old, center):
    return tuple(map(lambda x:x+center, old))

def has_decimal_part(fval):
    if fval - int(fval) > 0:
        return 1
    else:
        return 0

def create_adjacent_matrix_from(sentences, model):
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
    return mat


def resort(sentences):
    model = word2vecmain.train()
    mat = create_adjacent_matrix_from(sentences,model)
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

# resort(["windistance\n","dwm\n","C语言调用Python3实例_c调用python3_C5DX的博客-CSDN博客 — Mozilla Firefox\n","c-project\n"])