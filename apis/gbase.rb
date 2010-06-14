require 'rubygems'
require 'httparty'
require 'singleton'

# == VertexDB API realization via HTTP
# requires HTTParty
# 
class GBase
  include HTTParty
  base_uri 'localhost:8080'
  format :json
  
  include Singleton
  
  for met in %w{mkdir rm read link write size select}
    class_eval "def self.#{met} (*args); self.instance.#{met} *args; end; \n"
  end
  
  # original api realization
  def mkdir node
    get node_name(node) + action(:mkdir)
  end
  
  def rm node, key
    get node_name(node) + action(:rm, :key => key)
  end
  
  def size node
    get node_name(node) + action(:size)
  end
  
  def link node, to_path, key
    get node_name(node) + action(:link, :key => key, :toPath => to_path)
  end
  
  def read node, key
    get node_name(node) + action(:read, :key => key)
  end
  
  def write node, key, mode, body
    post node_name(node) + action(:write, :key => key, :mode => mode), :body => body.to_s
  end
  
  def select node, op, options = {}
    options[:op] = op
    get node_name(node) + action(:select, options)
  end
  
  def backup!
    get '/' + action(:backup)
  end

  private
  # adds slash in the begining of the string if there is not yet
  # 'items' => '/items'
  # '/items' => '/items'
  def node_name path
    path.strip!
    path = '/' + path if path[0] != '/'
    path
  end

  # generate action argument
  # action(:read, :key => 'node') => '?action=read&key=node
  def action act, add_args = {}
    '?action=' + act.to_s.strip + args(add_args)
  end
  
  # {:key => 'position', :arg2 => 'arg_value'} => &key=position&position=arg_value
  def args ha
    return '' if ha.empty?
    '&' + ha.to_a.map {|key, val| key.to_s + '=' + val.to_s.strip }.join('&')
  end
  
  def get *args
    self.class.get *args
  end
  
  def post *args
    self.class.post *args
  end
end


class PrettyVertex
  def initialize
    @db = GBase.instance
  end
  
  # writes create path and write hash of attributes there
  def write path, content = {}
    @db.mkdir path
    links = []
    
    parser = Proc.new do |scope, hash|
      hash.each do |key, value|
        
        # run it recursive
        if value.is_a? Hash
          @db.mkdir join(scope, key.to_s)
          parser.call join(scope, key.to_s), value
          next
        end
        
        # if key starts with '_' - write value, else make links
        if key.to_s[0, 1] == '_'
          @db.write scope, key, 'set', value
        else
          # store links, becouse distanation node may not exists yet
          links << [join(scope, value.to_s), join(scope, key.to_s)]
        end
      end
    end
    
    # run recursive runner
    parser.call path, content
    # make links
    links.each {|p| link p[0], p[1] }
  end
  
  # create path of distanation and make link
  def link node, p2
    link_dir, link_name = split_path p2
    @db.mkdir link_dir unless link_dir == ''
    @db.link node, link_dir, link_name
  end
  
  # deletes key
  def delete path
    pre_path, key = split_path path
    @db.rm pre_path, key
  end
  
  # deletes all nodes
  def clear!
    @db.select '/', 'rm'
  end
  
  def join *paths
    parts = File.join(*paths).split '/'
    res = []
    parts.each do |part|
      if part == '..'
        res.pop
      else
        res << part
      end
    end
    res.join '/'
  end
  
  alias_method :rm, :delete
  
  private
  # '/path/to/item/name' => ['/path/to]/item', 'name']
  def split_path path
    pre_path = path[0, path.rindex('/').to_i]
    key = path[pre_path.size + 1, path.size - pre_path.size - 1]
    [pre_path, key]
  end
end
