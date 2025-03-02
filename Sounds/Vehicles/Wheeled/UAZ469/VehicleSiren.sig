AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 1
   name "SirenState"
   tl -200 0
   children {
    2
   }
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 2
   name "SirenState"
   tl 200 0
   input 1
  }
 }
 compiled IOPCompiledClass {
  visited {
   5 6
  }
  ins {
   IOPCompiledIn {
    data {
     1 2
    }
   }
  }
  outs {
   IOPCompiledOut {
    data {
     0
    }
   }
  }
  processed 2
  version 2
 }
}