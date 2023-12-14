# SO p1_proj

IST Operative Systems 23/24 1st project

## Run C Program on Unix OS

Use the Makefile command to run the program.

```bash
$$ make   //to Compile

$$ make clean   //to clean previous program dump files including ./jobs/*.out files

$$ make run  //to Run on Jobs diriectory

$$ make clean && make && make run  // to clean compile and run

$$ ./ems  //to Run

$$ ./ems ./jobs  //to Run on Jobs diriectory

```

## Tests

Benchmarking tests 
```bash
$$ ./benchmark.sh                //on main directory

or

$$ make clean

$$ make

$$ time ./ems ./jobs x           // x should be an Integer 
```


## Contributing

Nuno Florencia ist1105865  

Marcos Machado ist1106082



