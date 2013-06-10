#!/usr/bin/env ruby

require 'muni'
require 'viewpoint'
include Viewpoint::EWS
Viewpoint::EWS::EWS.endpoint = 'https://pod51043.outlook.com/ews/Exchange.asmx'
ExchAccount = File.read("/root/exchcred.txt")

Viewpoint::EWS::EWS.set_auth(ExchAccount.split("|")[0],ExchAccount.split("|")[1])
 
def replaceTimes(sourceCode, index)

	muniLine = ARGV[0].scan(/[0-9]+/)[0]
	
	# if muniLine is zero, then we don't want bus times to be checked and included in watch app
	if muniLine != "0" then
		r = ARGV[0].scan(/[0-9]+/)[2]=="1" ? Muni::Route.find(muniLine).inbound : Muni::Route.find(muniLine).outbound
		nextTime = Time.at(r.stop_at(ARGV[0].scan(/[0-9]+/)[1]).predictions.map(&:epochTime)[index].to_i/1000).utc + Time.zone_offset("PDT")
		sourceCode = sourceCode.gsub(/\*\*YEAR#{index}\*\*/, nextTime.year.to_s())
		sourceCode = sourceCode.gsub(/\*\*MONTH#{index}\*\*/, nextTime.month.to_s())
		sourceCode = sourceCode.gsub(/\*\*DAY#{index}\*\*/, nextTime.mday.to_s())
		sourceCode = sourceCode.gsub(/\*\*HOUR#{index}\*\*/, nextTime.hour.to_s())
		sourceCode = sourceCode.gsub(/\*\*MIN#{index}\*\*/, nextTime.min.to_s())
	else
		sourceCode = sourceCode.gsub(/\*\*YEAR#{index}\*\*/, "2000")
		sourceCode = sourceCode.gsub(/\*\*MONTH#{index}\*\*/, "1")
		sourceCode = sourceCode.gsub(/\*\*DAY#{index}\*\*/, "1")
		sourceCode = sourceCode.gsub(/\*\*HOUR#{index}\*\*/, "0")
		sourceCode = sourceCode.gsub(/\*\*MIN#{index}\*\*/, "0")		
	end
  
  return sourceCode

end

def insertSchedule(sourceCode)

	eventCode = ""
	calendar = Viewpoint::EWS::CalendarFolder.get_folder :calendar
	
	currentYear = (DateTime.now.to_time.utc + Time.zone_offset("PDT")).to_datetime.year()
	currentMon = (DateTime.now.to_time.utc + Time.zone_offset("PDT")).to_datetime.mon()
	currentDay = (DateTime.now.to_time.utc + Time.zone_offset("PDT")).to_datetime.day()
	
	todaysItems = calendar.items_between(DateTime.new(currentYear,currentMon,currentDay,0,0,0,'-7'),DateTime.new(currentYear,currentMon,currentDay,23,59,59,'-7'))
	
	todaysItems.each_with_index do |calEvent, index|
		eventCode = eventCode + "cal_event = &cal_events[#{index}];\n"
		eventStart = calEvent.start.to_time.utc + Time.zone_offset("PDT")
		eventCode = eventCode + "cal_event->eventTitle = \"#{eventStart.hour()}:%02d #{calEvent.subject}\\n\";\n" % eventStart.min()
		eventStartMin = (eventStart.year()-1900)*365*24*60 + eventStart.month()*31*24*60 + eventStart.day()*24*60 + eventStart.hour()*60 + eventStart.min()
		eventCode = eventCode + "cal_event->eventTimeMin = #{eventStartMin};\n"
		eventCode = eventCode + "cal_event->buzzed = 0;\n"
	end
	
	sourceCode = sourceCode.gsub(/\*\*NUMBER_OF_CAL_EVENTS\*\*/, todaysItems.length.to_s)
	sourceCode = sourceCode.gsub(/\*\*CODE_WITH_EVENTS\*\*/, eventCode)

	return sourceCode
	
  #need to insert number of events **NUM_EVENTS** 
  #cal_event = &cal_events[0];
  #cal_event->eventTitle = "18:00 Dinner at Ethiopic\n";
  #cal_event->eventTimeMin = 113*365*24*60 + 5*30*24*60 + 31*24*60 + 22*60 + 53;
  #cal_event->buzzed = 0;
  
end

sourceCode = File.read("/root/BusTimer/src/example_template.txt")

sourceCode = replaceTimes(sourceCode, 0)
sourceCode = replaceTimes(sourceCode, 1)

sourceCode = insertSchedule(sourceCode)

begin
  file = File.open("/root/BusTimer/src/Bustimer.c", "w")
  file.write(sourceCode)
rescue IOError => e
  puts "file writing error"
ensure
  file.close unless file == nil
end
