namespace our {
  class ServerComponent {
    private:
    float Health=100.0;
    public:
      static std::string getID();
      void deserialize(const nlohmann::json& data);
      void decreaseHealth(float amount);
      float getHealth();`
  };
}