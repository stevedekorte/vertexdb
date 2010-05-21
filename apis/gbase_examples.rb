require 'gbase'


base = PrettyVertex.new
base.clear!
base.write 'items/1', {:_name => 'Super car', :_about => 'Harder better faster stronger!', :_cost => 7000, :_count => 100}
base.write 'catalogs/1', {:_name => 'Cars'}
base.link 'items/1', 'catalogs/1/items/1'
base.link 'catalogs/1', 'items/1/catalog'

base.write 'people', {
  1 => {:_name => 'Poul', :has_wife => '../3', :has_chldren => {
      1 => '../2',
      2 => '../4'
    }
  },
  
  2 => {:_name => 'Jane', :has_husband => '../4', :has_father => '../1', :has_mother => '../3', :has_chldren => {
      1 => '../../5',
      2 => '../../6'
    }
  },
  
  3 => { :_name => 'Jessica', :has_husband => '../1', :has_chldren => {
      1 => '../../2',
      2 => '../../4'
    }
  },
  
  4 => {:_name => 'Harry', :has_wife => '../2', :has_father => '../1', :has_mother => '../3', :has_chldren => {
    1 => '../../5',
    2 => '../../6',
    3 => '../../7'
  }},
  
  5 => {:_name => 'Sarah', :has_mother => '../2', :has_father => '../4' },
  6 => {:_name => 'Matthew', :has_mother => '../2', :has_father => '../4'},
  7 => {:_name => 'Jack', :has_father => '../4'}
}