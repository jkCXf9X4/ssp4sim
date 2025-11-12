import pyssp4sim


sim = pyssp4sim.Simulator("./resources/embrace/embrace.json")
sim.init()
sim.simulate()

