#!/usr/bin/env ruby

require 'muni'

muniLine = ARGV[0].scan(/[0-9]+/)[0]

r = ARGV[0].scan(/[0-9]+/)[2]=="1" ? Muni::Route.find(muniLine).inbound : Muni::Route.find(muniLine).outbound

def replaceTimes(sourceCode, r, index)

  nextTime = Time.at(r.stop_at(ARGV[0].scan(/[0-9]+/)[1]).predictions.map(&:epochTime)[index].to_i/1000).utc + Time.zone_offset("PDT")

  sourceCode = sourceCode.gsub(/\*\*YEAR#{index}\*\*/, nextTime.year.to_s())
  sourceCode = sourceCode.gsub(/\*\*MONTH#{index}\*\*/, nextTime.month.to_s())
  sourceCode = sourceCode.gsub(/\*\*DAY#{index}\*\*/, nextTime.mday.to_s())
  sourceCode = sourceCode.gsub(/\*\*HOUR#{index}\*\*/, nextTime.hour.to_s())
  sourceCode = sourceCode.gsub(/\*\*MIN#{index}\*\*/, nextTime.min.to_s())
  
  return sourceCode

end

sourceCode = File.read("/root/BusTimer/src/example_template.txt")
sourceCode = replaceTimes(sourceCode, r, 0)
sourceCode = replaceTimes(sourceCode, r, 1)

begin
  file = File.open("/root/BusTimer/src/Bustimer.c", "w")
  file.write(sourceCode)
rescue IOError => e
  puts "file writing error"
ensure
  file.close unless file == nil
end
