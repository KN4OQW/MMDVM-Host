# Tests

Two suites in one binary.

* **Unit tests** build a `.ini` file in a temporary directory, read it back
  through `CConf` and check what `CConfValidator` makes of it. They touch
  nothing outside their own files and run anywhere.
* **Bench tests** read the `.ini` file this machine actually runs with, check
  the modem device is present, and send a datagram to each enabled mode's
  gateway to see whether anything is listening. They only run when you ask for
  them.

## Building and running

```
make tests          # build and run the unit tests
make bench          # build and run everything, including the bench tests
```

Or drive the binary directly:

```
./MMDVM-Host-Test                                # unit tests only
./MMDVM-Host-Test --bench --ini /etc/MMDVM-Host.ini
./MMDVM-Host-Test --list                         # what is there
./MMDVM-Host-Test --filter dmr                   # just the DMR tests
```

| Option | |
| --- | --- |
| `--bench` | also run the tests that read the real `.ini` file and talk to the gateways |
| `--ini <file>` | which `.ini` file the bench tests read, default `MMDVM-Host.ini` |
| `--exclusive` | the host is stopped, so a network port that is already bound is a failure rather than a skip |
| `--timeout <ms>` | how long to wait for a gateway to answer, default 500 |
| `--filter <text>` | only run tests whose name contains this |
| `--verbose` | let the host's own logging through |
| `--list` | list the tests without running them |

The exit status is non-zero if anything failed, so this drops straight into a
service check or a CI job.

## On the bench

The gateway probes never bind the mode's own local port, so they are safe to
run with the host up:

```
./MMDVM-Host-Test --bench --ini /etc/MMDVM-Host.ini
```

The port tests are the exception, because they have to try the real port. With
the host running they report the port as in use and skip. To check the ports
properly, stop the host first and say so:

```
sudo systemctl stop mmdvmhost
./MMDVM-Host-Test --bench --exclusive --ini /etc/MMDVM-Host.ini
sudo systemctl start mmdvmhost
```

Each mode gets three bench tests:

* `bench.conf.<mode>` — everything the validator has to say about that mode as
  it is configured here. Warnings are printed; only failures fail the test.
* `bench.gateway.<mode>` — resolves the gateway address and sends it the poll
  the host would send. A gateway that is not running shows up as *unreachable*,
  which is an ICMP port unreachable coming back, and fails the test. A gateway
  that takes the datagram without answering passes, because several of these
  protocols never answer a poll.
* `bench.ports.<mode>` — can the mode's local port be bound.

A mode that is switched off, or whose network is switched off, is skipped
rather than failed.

### What "unreachable" can and cannot tell you

The probe connects the socket before sending, so the system reports an ICMP
port unreachable back to us. On a gateway running on this machine — which is
the usual arrangement — that is a solid answer: nothing is listening. For a
gateway across a network, a firewall that drops rather than rejects will show
as *no reply* instead, which the test lets pass. So a pass across the network
means "nothing said no", not "the gateway is definitely there".

## Adding a test

```c++
TEST("dmr.rejects_something")
{
        CIniBuilder ini = OnlyMode(CONF_MODE_DMR);
        ini.set("DMR", "ColorCode", "16");

        AssertFailure(Validate(ini, CONF_MODE_DMR), "[DMR] ColorCode");
}
```

`CIniBuilder` starts from a complete, valid configuration, so a test only has
to describe the one thing it wants wrong. `OnlyMode()` narrows that to a single
mode and its network.

Note that `remove()` and `set(..., "\"\"")` are not the same thing. `CConf`
throws away a key with nothing after the `=`, so `remove()` models an operator
leaving a line out and leaves the built in default in force, while `""` models
them deliberately blanking a value.

`BENCH_TEST` registers a test in the bench suite instead. Use `TestSkip()` when
there is nothing to check on this machine, and `TestNote()` to print something
the operator will want to see either way.

## What the validator checks

`ConfValidator.cpp` at the top level holds the rules, split by mode. Several of
them guard against an assertion inside the host rather than a clean failure —
`CDMRNetwork` asserts the DMR id is above 1000, and `CFMNetwork` asserts the FM
callsign is not empty, so either one aborts the whole host rather than
disabling the mode. Those are marked in the source.
