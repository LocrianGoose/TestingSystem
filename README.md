# TestingSystem
Simple Contest Testing system written in pure C.

Use ```$ ./build.sh``` to build and ```$ cd bin && ./judge contest``` to run the demo.

Working system consists of three components: ```judge```, ```tester``` and ```checkers``` directory with at least one checking program inside. All three components must be in the same folder.

Here is shown testing system with two sample contests:
```
├── judge
├── tester
├── checkers
│   ├── checker_byte
│   └── checker_int
|
├── SampleContest1
|   └──...
├── SampleContest2
|   └──...
⋮
```
In order to start testing of a contest in the ```contest_name``` folder run ```$ ./judge contest_name```\
In the example above SampleContest1 can be tested by running ```$ ./judge SampleContest1```

Additional information on ```TestingSystem``` such as contest folder structure is avaliable in the ```HELP.md```
