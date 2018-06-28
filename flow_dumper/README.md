Flow Dumper Application
=======================

What's this?
------------

This application retrieves flow entries from all switches connected and
outputs flow entries in text form on console terminal.

How to build
------------

  Get Trema and Apps

     $ sudo gem install trema
     $ git clone git://github.com/trema/apps.git apps

  Build flow dumper

     $ cd ./apps/flow_dumper
     $ make
     $ cd ../../

How to run
------------

  Run a controller application developed on top of Trema which installs
  flow entries. For example:

     $ trema run -c ./apps/routing_switch.conf -d

  Run flow dumper

     $ trema run ./apps/flow_dumper/flow_dumper

  Or

     $ trema run ./apps/flow_dumper/flow-dumper.rb

  If you have any flow entries, you can see output like follows:

     $ trema run ./apps/flow_dumper/flow_dumper
     [0x00000000000abc] priority = 65535, match = [wildcards = 0, in_port = 1,
     dl_src = 00:00:00:01:00:02, dl_dst = 00:00:00:01:00:01, dl_vlan = 65535,
     dl_vlan_pcp = 0, dl_type = 0x800, nw_tos = 0, nw_proto = 17,
     nw_src = 192.168.0.2, nw_dst = 192.168.0.1, tp_src = 1, tp_dst = 1],
     actions = [output: port=2 max_len=65535]
