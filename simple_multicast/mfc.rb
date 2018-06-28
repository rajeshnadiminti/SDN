
require "set"


#
# A database for multicast forwarding
#
class MFC
  def initialize
    @db = Hash.new do | hash, key |
      hash[ key ] = Set.new
    end
  end


  def learn group, port
    members( group ).add( port )
  end


  def remove group, port
    members( group ).delete( port )
  end


  def members group
    @db[ group.to_i ]
  end
end
