
public class MovementInfoT
{
  
  private int    id;
  private int    numFrames;
  private long   usecPerFrame;
  private long   usecTotal;
  private double beatsPerMinute;
  private long   usecPerBeat;
  
  public int    GetId()           { return id; }
  public int    GetNumFrames()    { return numFrames; }
  public long   GetUsecPerFrame() { return usecPerFrame; }
  public long   GetUsecTotal()    { return usecTotal; }
  public double GetBpm()          { return beatsPerMinute; }
  public long   GetUsecPerBeat()  { return usecPerBeat; }
  
  public void SetId(int val)            { id = val; }
  public void SetNumFrames(int val)     { numFrames = val; }
  public void SetUsecPerFrame(long val) { usecPerFrame = val; }
  public void SetUsecTotal(long val)    { usecTotal = val; }
  public void SetBpm(double val)        { beatsPerMinute = val; }
  public void SetUsecPerBeat(long val)  { usecPerBeat = val; }
}

public class MovementListT
{
  private final int ID_TOO_BIG = 1000000;
  
  private ArrayList<MovementInfoT> movements;
  
  public MovementListT()
  {
    movements = new ArrayList<MovementInfoT>();
  }
  
  public ArrayList<MovementInfoT> GetMovements()
  {
    return movements;
  }
  
  public int GetFirstClipID()
  {
    int id = ID_TOO_BIG;
    for (int i = 0; i < movements.size(); ++i)
    {
      if (movements.get(i).GetId() < id)
      {
        id = movements.get(i).GetId();
      }
    }
    
    if (id >= ID_TOO_BIG)
    {
      return CLIPID_NONE;
    }
    
    return id;
  }

  public int GetFirstClipIDInTheShow()
  {
    int id = ID_TOO_BIG;
    for (int i = 0; i < movements.size(); ++i)
    {
      if ((movements.get(i).GetId() >= CLIPID_FIRST_OF_SHOW)
        && (movements.get(i).GetId() < id))
      {
        id = movements.get(i).GetId();
      }
    }
    
    if (id >= ID_TOO_BIG)
    {
      return CLIPID_NONE;
    }
    
    return id;
  }
  
  public int GetLastClipID()
  {
    int id = CLIPID_NONE;
    for (int i = 0; i < movements.size(); ++i)
    {
      if (movements.get(i).GetId() > id)
      {
        id = movements.get(i).GetId();
      }
    }
    return id;
  }
  
  public int GetNextClipID(int clipID)
  {
    int id = ID_TOO_BIG;
    for (int i = 0; i < movements.size(); ++i)
    {
      if ((movements.get(i).GetId() > clipID)
        && (movements.get(i).GetId() < id))
      {
        id = movements.get(i).GetId();
      }
    }
    
    if (id >= ID_TOO_BIG)
    {
      return CLIPID_NONE;
    }
    
    return id;
  }
  
  MovementInfoT GetMovementInfoForClipID(int clipID)
  {
    for (int i = 0; i < movements.size(); ++i)
    {
      if (movements.get(i).GetId() == clipID)
      {
        return movements.get(i);
      }
    }
    return null;
  }
  
  int GetNumClips()
  {
    return movements.size();
  }
  
  MovementInfoT GetMovementInfoAtIndex(int index)
  {
    if ((index >= 0) && (index < movements.size()))
    {
      return movements.get(index);
    }
    
    return null;
  }
  
  boolean ReadMovementFile(String fname)
  {
    movements.clear();
    String lines[] = loadStrings(fname);
    for (int i = 0; i < lines.length; i++)
    {
      String words[] = lines[i].split("\\s+");
      
      if (words[0].equals("Movement") && words.length >= 5)
      {
        MovementInfoT m = new MovementInfoT();
        // id
        m.SetId(Integer.parseInt(words[1]));
        m.SetNumFrames(Integer.parseInt(words[2]));
        m.SetUsecPerFrame(Integer.parseInt(words[3]));
        m.SetBpm(Double.parseDouble(words[4]));
        
        double usecPerBeat = m.GetBpm();
        usecPerBeat /= 60.0;
        usecPerBeat = 1000000.0 / usecPerBeat;
        
        m.SetUsecTotal((long) m.GetNumFrames() * m.GetUsecPerFrame());
        m.SetUsecPerBeat((long) usecPerBeat); 
        movements.add(m);
      }
    }
    
    return true;
  }
}
