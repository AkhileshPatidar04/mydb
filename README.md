# mydb

A relational database engine built from scratch in C++ — page-based storage, buffer pool, B+ tree indexing, SQL parsing.


## storage 
![img](images/arch.png)

## Get(search) Record Flow in Storage Component
![Search](images/search1.png)
![Search](images/search2.png)

## Insert Record Flow in Storage Component
![Insert Opertion img](images/insert1.png)
![Insert Opertion img](images/insert2.png)

## Disk File Page Layout
![layout](images/insert2.png)

## Not started
- Row/schema serialization, SQL parser, planner, executor, TCP server, WAL, transactions, locking

## Build

```bash
mkdir build && cd build
cmake ..
make
```



## Design

See [`ARCHITECTURE.md`](ARCHITECTURE.md) for the full layered design and build-order plan.
