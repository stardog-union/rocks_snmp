# rocks_snmp
SNMP side channel for statistics retrieval from rocksdb

## Summary
This project creates an SNMP agent based upon RFC 2741.  The agent allows retrieval of rocksdb statistics without requiring the application that uses rocksdb to create any statistics infrastructure.

## Status
This is really old code.  It dates from a time when I was playing with templates and other new toys.  
It was used for monitoring Riak's leveldb, log files in production web environments, ArangoDB, and now Stardog.  
Some of the code is almost 20 years old and written for a RedHat 5 distribution.

The first release only supports rocksdb::Statistics as SNMP counters.  Expect other properties, 
including column family specific ones, to slowly creep into the code.

## Notes
It was easier to simply take the old code as is.  Only minimal updates / upgrades occurred before first release.
I also want to point out the new subsystems, such as libevent and cmake, are better for new projects.
