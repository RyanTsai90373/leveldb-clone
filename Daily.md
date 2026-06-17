## 6/17
- Progress
    1. complete log reader (but hasn't been tested)
    2. Rewrite the memtable format as [keySize|key(InternalKey)|tag|valueSize|value]
- Struggle
    1. does not understand how EncodeVariant32 can work. I thought it was about modifying a pointer, we should use ** to actually get the pointer we pass in, not a copied one.
    But, there are actually two (or more) patterns
        1. If you have to return something else, like return a status in GetWritableFile(), but you also want to get your pointer set, then you have to use ** to not pass by value.
        2. If you just have to return the data, you can use char* as parameter, then the function will get a copied pointer, but both are pointing to the same address. Therefore, the change made inside the function will apply to your pointer outside, even though the function never gets the real pointer.
    2. EncodeVariant32() and GetVariant32() both move the cursor for caller. Should treat a Slice in this context as a stream cursor, not a static window.
- Next