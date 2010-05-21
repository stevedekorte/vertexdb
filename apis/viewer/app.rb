require 'rubygems'
require 'sinatra'
require 'yajl'

require '../gbase'

set :views, File.expand_path(File.dirname(__FILE__))
set :public,   File.expand_path(File.dirname(__FILE__))

get '/' do
  File.open('./view.html') {|f| f.read }
end

get '/select' do 
  db = GBase.instance
  value = db.select params[:path], params[:op]
  content_type :json
  Yajl::Encoder.encode(value.clone)
end