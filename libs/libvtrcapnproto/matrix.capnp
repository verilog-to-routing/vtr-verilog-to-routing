@0xafffc9c1f309bc00;

struct Matrix(Value) {
    struct Entry {
        value @0 :Value;
    }
    dims @0 :List(Int64);
    data @1 :List(Entry);
}
