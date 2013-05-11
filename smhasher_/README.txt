Obtained from code.google.com/p/smhasher
and applied single change:
MurmurHash3.cpp:
 - #if defined(_MSC_VER)
 + #if defined(_MSC_VER) && (_MSC_VER <= 1500)