# -*- coding: utf-8 -*-
#
# Flow dumper
#



module Trema
  class FlowStatsReply
    def to_s
      "table_id = #{ table_id }, " +
      "priority = #{ priority }, " +
      "cookie = #{ cookie.to_hex }, " +
      "idle_timeout = #{ idle_timeout }, " +
      "hard_timeout = #{ hard_timeout }, " +
      "duration = #{ duration_sec }, " +
      "packet_count = #{ packet_count }, " +
      "byte_count = #{ byte_count }, " +
      "match = [#{ match.to_s }], " +
      "actions = [#{ actions.to_s }]"
    end
  end
end


class FlowDumper < Controller
  periodic_timer_event :timed_out, 2


  def start
    send_list_switches_request
  end

  
  def list_switches_reply switches
    request = FlowStatsRequest.new( :match => Match.new )
    switches.each do | each |
      send_message each, request
    end
    @num_switches = switches.size
  end


  def stats_reply datapath_id, message
    message.stats.each do | each |
      if each.is_a?( FlowStatsReply )
        info "[#{ datapath_id.to_hex }] #{ each.to_s }"
      end
    end

    @num_switches -= 1
    if @num_switches == 0
      shutdown!
    end
  end

  
  def timed_out
    shutdown!
  end
end


### Local variables:
### mode: Ruby
### coding: utf-8
### indent-tabs-mode: nil
### End:
