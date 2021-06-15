# rocks_snmp
SNMP side channel for statistics retrieval from rocksdb

## Summary
This project creates an SNMP agent based upon RFC 2741.  The agent allows retrieval of rocksdb statistics without requiring the application that uses rocksdb to create any statistics infrastructure.

## Status
WARNING:  This is threaded code that is now using std::shared_ptr.  std::shared_ptr is not generally thread safe.
Will update it with one of the 2 options available at C++11 level soon.  C++20 std::shared_ptr is not
widely available within standard distributions at this time.  There have been no known thread crashes due
to std::shared_ptr in this code.


This is really old code.  It dates from a time when I was playing with templates and other new toys.
It was used for monitoring Riak's leveldb, log files in production web environments, ArangoDB, and now Stardog.
Some of the code is almost 20 years old and written for a RedHat 5 distribution.

The first release only supports rocksdb::Statistics as SNMP counters.  Expect other properties,
including column family specific ones, to slowly creep into the code.

## Notes
It was easier to simply take the old code as is.  Only minimal updates / upgrades occurred before first release.
I also want to point out the new subsystems, such as libevent and cmake, are better for new projects.

## Example collectd.conf settings
<pre>
LoadPlugin snmp

&lt;Plugin snmp&gt;
	&lt;Data "stardog_table_1"&gt;
		Type "counter"
		Table true
		InstancePrefix "data_statistics"
		Instance ".1.3.6.1.4.1.38693.5.1.2"
		Values   ".1.3.6.1.4.1.38693.5.1.1"
	&lt;/Data&gt;
	&lt;Host "server.example.com"&gt;
		Address "127.0.0.1"
		Version 2
		Community "public"
		Collect "stardog_table_1"
		Interval 300
	&lt;/Host&gt;
&lt;/Plugin&gt;
</pre>
