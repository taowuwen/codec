
class StateMachine:

    def __init__(self, initialState):

        self.currentState = initialState
        self.currentState.run()

    def runAll(self, inputs):

        for i in inputs:
            print(i)

            self.currentState = self.currentState.next(i)
            self.currentState.run()
