static const char *const *
ReadFixPins(IN const char *const *Args,
	    OUT char **PinFile)
{
    enum e_OptionArgToken Token;
    int Len;
	const char *const *PrevArgs = Args;

    Args = ReadToken(Args, &Token);
    if(OT_RANDOM != Token)
	{
	    Len = 1 + strlen(*PrevArgs);
	    *PinFile = (char *)my_malloc(Len * sizeof(char));
	    memcpy(*PinFile, *PrevArgs, Len);
	}
    return Args;
}
