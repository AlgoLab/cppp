#!/usr/bin/env ruby
n = 0
m = 0
lines = []
srand
ARGF.each_line do |line|
  line =~ /(\d+)\s+(\d+)/
  c, string = $1, $2
  count = Integer(c)
  characters = string.split(//)
  m = string.size
  n += count
  while (count>1)
    pos = rand(m)
    new = characters.each_with_index.map { |x, i| i==pos ? (1-x.to_i).to_s : x }
    lines.push(new.join(' '))
    count = count - 1
  end
  lines.push(characters.join(' '))
end

puts "#{n} #{m}"
puts ""
lines.each {|l| puts l}
