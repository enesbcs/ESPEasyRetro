#ifndef COMMAND_TASKS_H
#define COMMAND_TASKS_H


String Command_Task_Clear(struct EventStruct *event, const char* Line)
{
	// Par1 is here for 1 ... TASKS_MAX
	taskClear(event->Par1 - 1, true);
	return return_command_success();
}

String Command_Task_ClearAll(struct EventStruct *event, const char* Line)
{
	for (byte t = 0; t < TASKS_MAX; t++)
		taskClear(t, false);
	return return_command_success();
}

String Command_Task_ValueSet(struct EventStruct *event, const char* Line)
{
	char TmpStr1[INPUT_COMMAND_SIZE];
	if (GetArgv(Line, TmpStr1, 4)) {
		float result = 0;
		Calculate(TmpStr1, &result);
		UserVar[(VARS_PER_TASK * (event->Par1 - 1)) + event->Par2 - 1] = result;
	}else  {
		//TODO: Get Task description and var name
		Serial.println(UserVar[(VARS_PER_TASK * (event->Par1 - 1)) + event->Par2 - 1]);
	}
	return return_command_success();
}

String Command_Task_ValueSetAndRun(struct EventStruct *event, const char* Line)
{
	char TmpStr1[INPUT_COMMAND_SIZE];
	if (GetArgv(Line, TmpStr1, 4)) {
		float result = 0;
		Calculate(TmpStr1, &result);
		UserVar[(VARS_PER_TASK * (event->Par1 - 1)) + event->Par2 - 1] = result;
		SensorSendTask(event->Par1 - 1);
	}
	return return_command_success();
}

String Command_Task_Run(struct EventStruct *event, const char* Line)
{
	SensorSendTask(event->Par1 - 1);
	return return_command_success();
}

String Command_Task_RemoteConfig(struct EventStruct *event, const char* Line)
{
	struct EventStruct TempEvent;
	TempEvent.TaskIndex = event->TaskIndex;
	String request = Line;
	remoteConfig(&TempEvent, request);
	return return_command_success();
}

#endif // COMMAND_TASKS_H
