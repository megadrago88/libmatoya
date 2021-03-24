
static void* struct_thread(void* opaque)
{
    //TODO: Push to queue
    return NULL;
};

static bool struct_main(void)
{
    const char* stringkey = "I'm a test string key!";
    const void* intvalue = 0xDEADBEEF;

    MTY_Hash* hashctx = MTY_HashCreate(0);
    test_cmp("MTY_HashCreate", hashctx != NULL);

    void *value = MTY_HashSet(hashctx, stringkey, &stringkey);
    test_cmp("MTY_HashSet (New)", value == NULL);

    value = MTY_HashSet(hashctx, stringkey, stringkey);
    test_cmp("MTY_HashSet (OW)", value && !strcmp(stringkey,*(char**)value));

    value = MTY_HashGet(hashctx, stringkey);
    test_cmp("MTY_HashGet", value && !strcmp(stringkey, value));

    value = MTY_HashSetInt(hashctx, 1, &intvalue);
    test_cmp("MTY_HashSetInt (New)", value == NULL);

    value = MTY_HashSetInt(hashctx, 1, intvalue);
    test_cmp("MTY_HashSetInt (OW)", value && *(void**)value == intvalue);

    value = MTY_HashGetInt(hashctx, 1);
    test_cmp("MTY_HashGetInt", value && value == intvalue);

    uint64_t iter = 0;
    int64_t popintkey = 0;
    const char* popkey;
    bool r = MTY_HashGetNextKey(hashctx, &iter, &popkey);
    test_cmpi64("MTY_HashGetNextKey (I)", r, iter);

    r = MTY_HashGetNextKey(hashctx, &iter, &popkey);
    test_cmpi64("MTY_HashGetNextKey (S)", r, iter);

    iter = 0; //have to reset since the KeyInt function will fail on the normal string key
    r = MTY_HashGetNextKeyInt(hashctx, &iter, &popintkey);
    test_cmpi64("MTY_HashGetNextKeyInt (I)", r, iter);

    r = MTY_HashGetNextKeyInt(hashctx, &iter, &popintkey);
    test_cmpi64("MTY_HashGetNextKeyInt (S)", !r, iter);

    value = MTY_HashPop(hashctx, popkey);
    test_cmp("MTY_HashPop", value && !strcmp(stringkey, value));

    value = MTY_HashPopInt(hashctx, popintkey);
    test_cmp("MTY_HashPopInt", value && value == intvalue);

    MTY_HashDestroy(&hashctx, NULL);
    test_cmp("MTY_HashDestroy", hashctx == NULL);

    MTY_Thread* thread = MTY_ThreadCreate(struct_thread, stringkey);

    MTY_Queue* queuectx = MTY_QueueCreate(2, 4);
    test_cmp("MTY_QueueCreate", queuectx != NULL);

    //TODO: Add tests to these (use a thread for adaquete testing)
    //value = MTY_QueueAcquireBuffer(queuectx);
    //MTY_QueueFlush(queuectx, NULL);
    //MTY_QueueGetLength(queuectx);
    //MTY_QueuePush(queuectx, NULL);
    //r = MTY_QueuePushPtr(queuectx, NULL, NULL);
    //r = MTY_QueuePop(queuectx, NULL, NULL, NULL);
    //r = MTY_QueuePopPtr(queuectx, NULL, NULL, NULL);
    //r = MTY_QueuePopLast(queuectx, NULL, NULL, NULL);
    //MTY_QueueReleaseBuffer(queuectx);

    MTY_QueueDestroy(&queuectx);
    test_cmp("MTY_QueueDestroy", queuectx == NULL);

    MTY_ThreadDestroy(&thread);

    MTY_List* listctx = MTY_ListCreate(); //TODO: Go back and add more tests
    test_cmp("MTY_ListCreate", listctx != NULL);

    MTY_ListNode* node = MTY_ListGetFirst(listctx);
    test_cmp("MTY_ListGetFirst (Empty)", node == NULL);

    MTY_ListAppend(listctx, intvalue);
    node = MTY_ListGetFirst(listctx);
    test_cmp("MTY_ListAppend", node != NULL);

    value = MTY_ListRemove(listctx, node);
    node = MTY_ListGetFirst(listctx);
    test_cmp("MTY_ListRemove", value && value == intvalue);

    MTY_ListDestroy(&listctx, NULL);
    test_cmp("MTY_ListDestroy", listctx == NULL);

    return true;
}
