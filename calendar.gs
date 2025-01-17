function doGet(e) {
  var date = e.parameter.theDate;
  return HtmlService.createHtmlOutput(getEvents(date)); // textOutput redirects to a new url, which a Giga can't follow
}

function getEvents(calDate) {
  var _calendarId = 'the id of the calendar'
  var Cal = CalendarApp.getCalendarById(_calendarId);
  var date = new Date(parseInt(calDate));
  var events = Cal.getEventsForDay(date);
  str = "%!!!"; // I add this character string so that the arduino code can find the useful information
  for (var i = 0; i < events.length; i++) {
    if (!events[i].isAllDayEvent()) { // you can include these if you want, but it makes the calendar line unhelpful, which is why I excluded them
      str += events[i].getTitle() + ' (' + events[i].getStartTime().getTime() + ' - ' + events[i].getEndTime().getTime() + '); ' ;
    }
  }
  str += "!!!%";
  return str;
}
