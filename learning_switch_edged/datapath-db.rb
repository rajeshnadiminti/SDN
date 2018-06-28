class DatapathEntry
  attr_reader :id
  attr_reader :fdb


  def initialize datapath_id, fdb
    @id = datapath_id
    @fdb = fdb
  end
end


class DatapathDB
  include Trema::Logger


  def initialize
    @db = {}
  end


  def insert datapath_id, fdb
    debug "insert switch: #{ datapath_id }"
    if @db[ datapath_id ]
      warn "duplicated switch ( datapath_id = #{ datapath_id } )"
    end
    @db[ datapath_id ] = DatapathEntry.new( datapath_id, fdb )
  end


  def lookup datapath_id
    debug "lookup switch: #{ datapath_id }"
    @db[ datapath_id ]
  end


  def delete datapath_id
    debug "delete switch: #{ datapath_id }"
    @db.delete( datapath_id )
  end


  def each_value &block
    @db.each_value do | each |
      block.call each.fdb
    end
  end
end

