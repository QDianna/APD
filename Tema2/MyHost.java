import java.util.LinkedList;
import java.util.Queue;

public class MyHost extends Host {
    private Queue<Task> taskQueue = new LinkedList<>();
    private Object queueLock = new Object();
    private boolean isRunning = true;
    private boolean isExecuting = false;
    private long leftRunningTask = 0;

    @Override
    public void run() {
        while (isRunning) {
            Task currentTask;

            synchronized (queueLock) {
                // make thread wait until it gets a task to execute
                while (taskQueue.isEmpty() && isRunning) {
                    try {
                        queueLock.wait();

                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        System.out.println("*** Thread interrupted while waiting. Exiting.");
                        e.printStackTrace();
                    }
                }
                
                // check if thread was stopped from 'shutdown()' before getting notified
                if (!isRunning)
                    break;
                
                // extract task from queue
                currentTask = taskQueue.poll();
            }

            // start execution of current task
            isExecuting = true;
            leftRunningTask = 0;
            executeTask(currentTask);
        }
    }


    private void executeTask(Task task) {
        // long startTime = task.getStart() * 1000L;
        long startTime = System.currentTimeMillis();
        long duration = task.getLeft();
        long elapsedTime = 0;

        // System.out.println("Executing -TASK " + task.getId() + "-");

        while (elapsedTime < duration) {
            // check if I need to preempt current running task
            if (task.isPreemptible() && findHigherPrio(task.getPriority())) {
                // System.out.println("!!! Found higher prio task than running task");
                
                // add task back to queue with it's remaining time updated
                task.setLeft(duration - elapsedTime);
                isExecuting = false;

                // System.out.println("Preempting -TASK " + task.getId() + "-");
                addTask(task);
                break;
            }
            
            // simulate the execution of current task
            try {
                Thread.sleep(100);

            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                System.out.println("*** Thread interrupted while waiting. Exiting.");
                e.printStackTrace();
            }
            
            // update current task's time left to execute
            elapsedTime = System.currentTimeMillis() - startTime;
            // variable used in getWorkLeft() to add the time left for current task
            leftRunningTask = (duration - elapsedTime);
        }

        // mark task as finished
        if (elapsedTime >= duration) {
            // long ts = (elapsedTime) / 1000;
            // System.out.println("* Marking finished -TASK " + task.getId() + "- Timestamp(s) = " + ts);
            isExecuting = false;
            task.finish();
        }
    }


    private boolean findHigherPrio(int currentTaskPrio) {
        if (taskQueue.isEmpty())
            return false;

        for (Task t : taskQueue) {
            if (t.getPriority() > currentTaskPrio) {
                return true;
            }
        }

        return false;
    }

    @Override
    public void addTask(Task task) {
        synchronized (queueLock) {
            long left = task.getLeft();
            long duration = task.getDuration();
            // if executing time left and duration are not the same, the task has
            // already been partially executed, therefore is a pre-empted task
            boolean wasPreempted = !(left == duration);
            // System.out.println("? Preempted task = " + wasPreempted);

            // System.out.print("Adding to queue -TASK " + task.getId());

            if (taskQueue.isEmpty()) {
                // System.out.println("- on first position");
                taskQueue.add(task);
            
            } else {
                if (!wasPreempted) {
                    // task was not preempted, put it after any other task with same prio
                    int index = 0;

                    for (Task t : taskQueue) {
                        if (t.getPriority() >= task.getPriority()) {
                            index++;

                        } else {
                            // System.out.println(" -on position " + index);
                            ((LinkedList<Task>) taskQueue).add(index, task);
                            break;
                        }
                    }

                    if (index == taskQueue.size()) {
                        // System.out.println("- on last position");
                        taskQueue.add(task);
                    }
                } else {
                    // task was preempted, put it before any other task with same prio
                    int index = 0;

                    for (Task t : taskQueue) {
                        if (t.getPriority() > task.getPriority()) {
                            index++;
                        } else {
                            // System.out.println("- on position " + index);
                            ((LinkedList<Task>) taskQueue).add(index, task);
                            break;
                        }
                    }

                    if (index == taskQueue.size()) {
                        // System.out.println("- on last position");
                        taskQueue.add(task);
                    }
                }
            }

            // System.out.print("* Queue = [");
            // for (Task t : taskQueue) {
                // System.out.print(t.getId() + ", ");
            // }
            // System.out.println("]");
            
            queueLock.notify();
        }
    }

    @Override
    public int getQueueSize() {
        if (isExecuting)
            return taskQueue.size() + 1;
        
        return taskQueue.size();
    }

    @Override
    public long getWorkLeft() {
        long workLeftMili = 0;
        long workLeftSeconds = 0;

        for (Task t : taskQueue) {
            // convert milliseconds to seconds
            workLeftMili += t.getLeft();
        }

        if (isExecuting) {
            // convert milliseconds to seconds
            workLeftMili += leftRunningTask;
        }

        workLeftSeconds = workLeftMili / 1000L;

        // System.out.println("--- Work left for -HOST  " + this.getId() + "- is = " + workLeftSeconds);
        return workLeftSeconds;
    }


    @Override
    public void shutdown() {
        // System.out.println("******* Shutting down -HOST " + this.getId() + "- *******");
        isRunning = false;
        synchronized (queueLock) {
            // notify the waiting thread to check the exit condition
            queueLock.notify();
        }
    }
}
