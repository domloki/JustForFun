import numpy as np
import itertools as it
from math import factorial
import re
import sys

# Simple funtion to calcualte permutation nPm
def permutation(m, n):
    return factorial(n) / (factorial(n - m) * factorial(m))

# To find number of iteration to do no of colums of A * m
def matrix_combinations(matr, n):
    timed = list(map(list, it.combinations(matr, n)))
    return np.array(list(timed))

# To Find number of combination to itterate size of B and Colums of B Ax<=B
def array_combinations(arr, n):
    timed = list(map(list, it.combinations(arr, n)))
    return np.array(list(timed))

# To type check inequality
def check_extreme(matr, arr, x, sym_comb, m):
    sym_comb = sym_comb.replace(']', '')
    sym_comb = sym_comb.replace('[', '')
    sym_comb = re.split("[ ,]", sym_comb)
    for i in range(int(m)):
        td_answer = sum(matr[i] * x)
        if sym_comb[i] == '>':
            if td_answer <= arr[i]:
                return 0
        elif sym_comb[i] == '>=':
            if td_answer < arr[i]:
                return 0
        elif sym_comb[i] == '<':
            if td_answer >= arr[i]:
                return 0
        elif sym_comb[i] == '<=':
            if td_answer > arr[i]:
                return 0
        elif sym_comb[i] == '=':
            if td_answer != arr[i]:
                return 0
        elif sym_comb[i] == '!=':
            if td_answer == arr[i]:
                return 0
        else:
            return 0
    return 1


def extreme_points(A, b, sym_comb):
    # Input
    A = np.array(A)
    b = np.array(b)
    m, n = A.shape
    # Process
    ans_comb = np.zeros((1, n))
    arr_comb = array_combinations(b, n)
    matr_comb = matrix_combinations(A, n)
    for i in range(int(permutation(n, m))):
        if np.linalg.det(matr_comb[i]) != 0:
            x = np.linalg.solve(np.array(matr_comb[i], dtype='float'),
                                np.array(arr_comb[i], dtype='float'))
            ans_comb = np.vstack([ans_comb, x])
    ans_comb = np.delete(ans_comb, 0, axis=0)
    j = 0
    for i in range(len(ans_comb)):
        if check_extreme(A, b, ans_comb[j], sym_comb, m):
            ans_comb = ans_comb
            j += 1
        else:
            ans_comb = np.delete(ans_comb, j, axis=0)
    # Output
    return ans_comb

def constraintStastification(A,B,C,ans):
    m = A.shape[0] # To get number of rows
    n = A.shape[1] # To get number of columns

    fism = ans.shape[0] # To get row dimension of extreme point matrix
    fisn = ans.shape[1] # To get column dimension of extreme point matrix

    # Tracking variables
    res = 0
    flag = 0
    max = 0

    # Assuming Number of non degenerated case are possible
    for i in range (fism):
        for j in range (m):
            for k in range (n):
                res += A[j][k] * ans[i][k]
            if ( res > B[j] ):
                flag = 1
                break
            res = 0
        if ( flag == 0 ):
            cost = 0
            for z in range (fisn):
                cost += C[z] * ans[i][z]
            if ( cost > max ):
                max = cost
        if (flag==1):
            flag = 0
            continue

    print ("Possible vector (if more then one vector then all might not be feasible)")
    if ( max > 0 ):
	    for i in range (fism):
		    for j in range (fisn):
			    print (ans[i][j])

    if ( max > 0 ):
        print ("feasible solution is possible and max value is")
        print (max)
        
    

#Taking Input
print ("Enter number of rows")
m = int(input()) # Number of rows
print ("Enter number of coumns")
n = int(input()) # Number of columns

print ("Enter row wise elements of A, seperated by space (enter all rows in single line)")
# matrix input, single line seprated by space AB <= C for A n*m
entries = list(map(int, input().split()))

# To convert into matrix form
A = np.array(entries).reshape(m,n)

print ("Enter values of column B, seperated by space")
# matrix input, single line seprated by space Ax <= 0 for B x 1*m
B = list(map(int, input().split()))
# Convert list to array
B = np.array(B).reshape(m)


print ("Enter values of column C, seperated by space")
# matrix input, single line seprated by space Ax <= 0 for C 1*n
C = list(map(int, input().split()))
# Convert list to array
C = np.array(C).reshape(n)

# if matrix member are out of bounds
for i in range (len(C)):
    if (C[i] < 0):
        print ("No fesiable solution possible")
        exit()

# We are assuming all constraints are in form of <= 
Rel = '['
for i in range(m-1):
    Rel += '<='
    Rel += ','
Rel += '<='
Rel += ']'

ans = extreme_points(A, B, Rel)

ans2=[]
# Removing redudant boundary points which are out of bound
for i in range (ans.shape[0]):
    for j in range (ans.shape[1]):
        if ( ans[i][j] < 0 ):
            ans2.append(i)
            break

for i in range (len(ans2)):
    np.delete(ans,ans2[i],0)

if ( ans.shape[0] == 0 or ans.shape[1] == 0 ):
    printf ("No feasiable solution possible")
else:
    constraintStastification ( A, B, C, ans)
