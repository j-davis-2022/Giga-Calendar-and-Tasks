// doGet() has to be commented out for doPost() deployments; I put both functions here anyway
function doGet() {
  var tasks = Tasks.Tasks.list("id of task list"); // you need to supply the id yourself
  if (!tasks.items) {
    str = "%!!!No tasks found.!!!%";
    return HtmlService.createHtmlOutput(str);
  }
  str = "%!!!"; // this string of characters makes it easy for the arduino code to find what it needs
  for (let i = (tasks.items.length - 1); i >= 0; i--) {
    const task = tasks.items[i];
    if (task.status != "completed") {
      str = str + task.title + "(" + task.id + "); ";
    }
  }
  str += "!!!%";
  return HtmlService.createHtmlOutput(str);
}

function doPost(e) {
  var params = e.postData.contents;
  var completedTaskId = params.substring(7, 29);
  var completed = params.substring(46, 50);
  var task = Tasks.newTask();
  if (completed == "0") {
    task.setStatus('needsAction');
  } else {
    task.setStatus('completed');
  }
  Tasks.Tasks.patch(task, "id of task list", completedTaskId);
  return HtmlService.createHtmlOutput("%!!!" + completedTaskId + "; " + completed + "!!!%");
}
