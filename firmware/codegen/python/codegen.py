#!/usr/bin/python

import sys

debug = False
#debug = True

def ror(code,n):
    # rotate right (digitwise) (assuming 12 digits):
    # n = number of shifts
    #print "code=",code
    for i in range(n):
        m = code % 10
        c = code / 10
        c+=((10**11)*m)
        code = c
        #print "code=",code
    return code

def rol(code,n):
    # rotate left (digitwise) (assuming 12 digits):
    # n = number of shifts
    #print "code=",code
    for i in range(n):
        m = code / 100000000000
        #print "m=",m
        c = code % 100000000000
        #print "c=",c
        c *= 10
        code = c+m
        #print "code=",code
    return code



lut=[ 35 , 86 , 69 , 93 , 99 , 63 , 74 , 15 , 84 , 88 ,
82 , 26 , 50 , 43 , 58 , 55 , 97 , 71 , 52 , 96 ,
13 , 67 , 56 , 20 , 27 , 46 , 9 , 19 , 73 , 38 ,
1 , 2 , 77 , 64 , 5 , 57 , 3 , 25 , 21 , 31 ,
61 , 16 , 8 , 75 , 24 , 92 , 78 , 30 , 33 , 6 ,
10 , 44 , 72 , 83 , 53 , 95 , 49 , 11 , 18 , 42 ,
40 , 28 , 76 , 17 , 0 , 14 , 12 , 80 , 47 , 87 ,
68 , 98 , 32 , 51 , 39 , 81 , 62 , 34 , 45 , 54 ,
85 , 36 , 4 , 37 , 41 , 66 , 91 , 70 , 89 , 60 ,
79 , 94 , 29 , 90 , 22 , 7 , 65 , 48 , 59 , 23 ]

digitlut= [1, 7, 2, 6, 4, 0, 9, 8, 5, 3]

def crypt_digits(code):
    newcode=0
    for i in range(14):
        m = code % 10
        code=code / 10
        cm = digitlut[m]
        #print "m=",m,"cm=",cm
        newcode+=cm*10**i
    return newcode

def decrypt_digits(code):
    newcode=0
    for i in range(14):
        m = code % 10
        code=code / 10
        cm = digitlut.index(m)
        #print "m=",m,"cm=",cm
        newcode+=cm*10**i
    return newcode

def generate_rcode(code,seq):
    lutseq=lut[seq]
    c=code/100
    code=ror(c,lutseq)
    newcode=code*100+lutseq
    return crypt_digits(newcode)

def decode_rcode(code):
    code=decrypt_digits(code)
    seq=code%100
    c=code/100
    code=rol(c,seq)
    newcode=code*100+lut.index(seq)
    return newcode



seq=0
runs=100

if len(sys.argv) == 4:
    seq=int(sys.argv[3]);
    runs=1
elif len(sys.argv) < 3:
    print "Usage: %s id days seq\n" % sys.argv[0]
    print "Example: %s 372759817 5 1" % sys.argv[0]
    print "(if seq not given, it prints 100 codes)"
    sys.exit();

days = int(sys.argv[2])
shsid = int(sys.argv[1]) # my SHS. Use code *1# to readout from keypad

if shsid>999999999 or days>999 or seq>99:
    print "Number too large (shsid<1000000000 days<1000 seq<100)"
    sys.exit();

for i in range(runs):
    code = shsid * 100000
    code = code + (days * 100)
    code = code + seq

    inputcode=code
    cryptcode = generate_rcode(code,seq)
    print "codegen id=%09d days=%03d seq=%02d ==> code = %014lu -> %014lu" % (shsid,days,seq,inputcode,cryptcode)
    code = decode_rcode(cryptcode)
    outputcode=code
    #print "decode = %014lu" % (code)
    seq = seq + 1

    if inputcode != outputcode:
        print "inputcode != outputcode"
        sys.exit();

